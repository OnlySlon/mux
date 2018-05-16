#define _GNU_SOURCE
#include <signal.h>
#include <sched.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <errno.h>
#include <poll.h>
#include <sys/time.h>
#include <time.h>
#include <sys/socket.h>
#include <ctype.h>

#include "mux.h"

extern CLOG_INFO *info;


/*
    Semi-ready APR stack
    TODO
    	- Enque unresolved packets
    	- UNICAST/BROADCAST arp resolving for RFC compatible
*/


enum etharp_state
{
	ETHARP_STATE_PENDING = 0,
	ETHARP_STATE_STABLE,
	ETHARP_STATE_STABLE_REREQUESTING_1,
	ETHARP_STATE_STABLE_REREQUESTING_2,
	ETHARP_STATE_STATIC
};



#define ARP_MAXAGE 100
/** Re-request a used ARP entry 1 minute before it would expire to prevent
 *  breaking a steadily used connection because the ARP entry timed out. */
#define ARP_AGE_REREQUEST_USED_UNICAST   (ARP_MAXAGE - 30)
#define ARP_AGE_REREQUEST_USED_BROADCAST (ARP_MAXAGE - 15)

#define ARP_MAXPENDING 5



struct arp_record *arpdb = NULL;



int 
etharp_raw(struct netif *netif, const struct eth_addr *ethsrc_addr,
		   const struct eth_addr *ethdst_addr,
		   const struct eth_addr *hwsrc_addr, const u_int32_t *ipsrc_addr,
		   const struct eth_addr *hwdst_addr, const u_int32_t *ipdst_addr,
		   const u_int16_t opcode)
{
	int result = 0;
	struct etharp_hdr *hdr;
	char *pkt;

	pkt = malloc(SIZEOF_ETH_HDR + SIZEOF_ETHARP_HDR);
	if (!pkt)
		return -1;

	hdr           = (struct etharp_hdr *) (pkt + SIZEOF_ETH_HDR);
	hdr->opcode   = htons(opcode);

	/* Write the ARP MAC-Addresses */
	memcpy(&hdr->shwaddr.addr, hwsrc_addr, ETH_HWADDR_LEN);
	memcpy(&hdr->dhwaddr.addr, hwdst_addr, ETH_HWADDR_LEN);
	/* Write the IP address */
	memcpy(&hdr->sipaddr, ipsrc_addr, 4);
	memcpy(&hdr->dipaddr, ipdst_addr, 4);

	hdr->hwtype   = htons(IANA_HWTYPE_ETHERNET);
	hdr->proto    = htons(ETHTYPE_IP);
	/* set hwlen and protolen */
	hdr->hwlen    = ETH_HWADDR_LEN;
	hdr->protolen = 4;


//	ethernet_output(netif, p, ethsrc_addr, ethdst_addr, ETHTYPE_ARP);
	free(pkt);
	return result;


}

/* add/update ARP entry function */
struct arp_record *mux_arpdb_update(struct eth_addr *eaddr, 
									u_int32_t        ip,
									MuxIf_t         *ifr,
									u_int32_t        state)
{
	ArpRecord *s;    
	struct in_addr ipaddr;
	int hash = (int) ip;

	HASH_FIND_INT(arpdb, &hash, s);	 /* id already in the hash? */
	if (s == NULL)
	{

		ipaddr.s_addr = ip;
		s = (ArpRecord *) malloc(sizeof(ArpRecord));
		memset(s, 0, sizeof(ArpRecord));
		s->id        = (int) ip;
		s->ifr       = ifr;
		s->state     = state;
		s->ip.s_addr = ip;
		s->ctime     = 0;
		memcpy(s->arp.addr, eaddr->addr, ETH_HWADDR_LEN);
		clog(info, CMARK, DBG_SYSTEM, "F:%s: Create ARP ASSOC %02x:%02x:%02x:%02x:%02x:%02x <--> %s  IfName: '%s' %p",
			 __FUNCTION__, eaddr->addr[0], eaddr->addr[1], eaddr->addr[2], eaddr->addr[3], eaddr->addr[4], eaddr->addr[5], 
			 inet_ntoa(ipaddr), ifr->name,  s); 
		HASH_ADD_INT(arpdb, id, s);
		return s;
	}


	ipaddr.s_addr = ip;
	s->ifr        = ifr;
	s->state      = state;
	s->ctime      = 0;
	memcpy(s->arp.addr, eaddr->addr, ETH_HWADDR_LEN);
	clog(info, CMARK, DBG_SYSTEM, "F:%s: Update ARP ASSOC %02x:%02x:%02x:%02x:%02x:%02x <--> %s  IfName: '%s' state=%i",
		 __FUNCTION__, eaddr->addr[0], eaddr->addr[1], eaddr->addr[2], eaddr->addr[3], eaddr->addr[4], eaddr->addr[5], 
		 inet_ntoa(ipaddr), ifr->name,  state); 
	// arp enty already exist
	return NULL;

}

void mux_arpdb_send_request(MuxIf_t *txif,
							u_int32_t resolve_from,
							u_int32_t resolve_ip)
{
	char *pkt_out;
	struct eth_addr dstmac;
	pkt_out = malloc(0xFF);
	struct etharp_hdr *arp_request = (struct etharp_hdr *) (pkt_out + SIZEOF_ETH_HDR);


	arp_request->opcode    = ntohs(ARP_REQUEST);
	arp_request->hwtype    = ntohs(IANA_HWTYPE_ETHERNET);
	arp_request->hwlen     = ETH_HWADDR_LEN;
	arp_request->protolen  = 4;
	arp_request->proto     = ntohs(ETHTYPE_IP);
	arp_request->sipaddr   = resolve_from; // need setup self addr

	memset(&dstmac.addr, 0xFF, ETH_HWADDR_LEN );
	memcpy(&arp_request->shwaddr, &txif->hwaddr.addr, ETH_HWADDR_LEN);


	// Fill resolve mac is ZERO
	memset(&arp_request->dhwaddr, 0, ETH_HWADDR_LEN);
	// looking ip
	arp_request->dipaddr = resolve_ip;

	etharp_output(txif, pkt_out, &txif->hwaddr, &dstmac, ETHTYPE_ARP);                      
	clog(info, CMARK, DBG_SYSTEM, "F:%s:  ARP_REPLY XMIT!!!", __FUNCTION__);
	free(pkt_out);
}


struct arp_record *mux_arpdb_search(u_int32_t ip)
{
	struct arp_record *s;
	int key = (int) ip;
	HASH_FIND_INT(arpdb, &key, s);
	if (s)
	{

		return s;
	}
	else
	{
		return NULL;
	}

}


// Send/Resend arp request send helper for Existing ArpRecord
void mux_arpdb_sendreq(MuxIf_t   *txif,
					   ArpRecord *ar)
{
	/* try get IP source address for resolving */
	MuxIPCfg *rfrom = mux_ipconf_match4(txif->ip, ar->ip);
	if (rfrom)
	{
		mux_arpdb_send_request(txif, rfrom->ipaddr, ar->ip.s_addr);
	}
	else
	{
		clog(info, CMARK, DBG_SYSTEM, "F:%s: Can't find ARP resolution source address FIX-ME", __FUNCTION__);

	}
}


/* --- MAIN LOOKUP FUNCTION --- */
ArpRecord *mux_arpdb_resolve(u_int32_t  ip, 
							 MuxIf_t   *txif, 
							 char      *pkt,
							 u_int32_t  pkt_len)
{
	struct in_addr ipaddr;
	ipaddr.s_addr = ip;

	ArpRecord *s;



	clog(info, CMARK, DBG_SYSTEM, "F:%s: Try resolve: %s txif->ip=%p", __FUNCTION__,  inet_ntoa(ipaddr), txif->ip);
	s = mux_arpdb_search(ip);


	if (s)
	{
		if (s->state > ETHARP_STATE_PENDING)
		{
			//  --- arp record exist and not at pending state
			if (s->ctime >= ARP_AGE_REREQUEST_USED_BROADCAST)
			{	
				mux_arpdb_sendreq(txif, s);
				s->state = ETHARP_STATE_STABLE_REREQUESTING_1;
			}
/* 
			TODO: UNICAST Resolving 
			if (s->ctime >= ARP_AGE_REREQUEST_USED_UNICAST)
			{
			}

*/

			return s;
		}
		// record exist but in PENGING mode
		clog(info, CMARK, DBG_SYSTEM, "F:%s: Record exist in PENDING state (%u)", __FUNCTION__, s->state);
		goto enqueue;

	}
	else
	{
		MuxIPCfg *rfrom = mux_ipconf_match4(txif->ip, ip);
		if (rfrom)
		{
			clog(info, CMARK, DBG_SYSTEM, "F:%s: NEW RECORD!", __FUNCTION__);
			// record no fucking exist at all
			struct eth_addr addmac;
			memset(&addmac.addr, 0xFF, ETH_HWADDR_LEN); 
			mux_arpdb_update(&addmac, ip, txif, ETHARP_STATE_PENDING);
			// kickstart first resolve try;
//			mux_arpdb_send_request(txif, rfrom->ipaddr, ip);
		}
		else
		{
			clog(info, CMARK, DBG_SYSTEM, "F:%s: Can't find ARP resolution source address", __FUNCTION__);

		}
	}

	enqueue:
	{
		// enqueue incoming unresolved packet
		// TODO. Now just drop
	}
	return NULL;
}

/* clean all arp records for cpecific interface  */
void mux_arpdb_cleanup(MuxIf_t *ifr)
{
	struct arp_record *s;

	for (s=arpdb; s != NULL; s = (struct arpdb_record *)(s->hh.next))
	{
		if (s->ifr == ifr)
		{
			HASH_DEL(arpdb, s);
			free(s);
		}
	}
	return;

}




void etharp_input(MuxIf_t *ifr, char *pkt, u_int32_t len)
{
	struct eth_hdr    *eth = (struct eth_hdr *) pkt;
	struct etharp_hdr *arp = (struct etharp_hdr *) (pkt + sizeof(struct eth_hdr));
	u_int32_t sipaddr, dipaddr;
	u_int32_t for_us;

	/* RFC 826 "Packet Reception": */
	if ((arp->hwtype != htons(IANA_HWTYPE_ETHERNET)) ||
		(arp->hwlen != ETH_HWADDR_LEN) ||
		(arp->protolen != 4) ||
		(arp->proto != htons(ETHTYPE_IP)))
	{
		clog(info, CMARK, DBG_SYSTEM, "F:%s: Something wrong with ARP packet", __FUNCTION__);
		return;
	}

	memcpy(&sipaddr, &arp->sipaddr, 4);
	memcpy(&dipaddr, &arp->dipaddr, 4);

	int opcode = (int) htons(arp->opcode);
	switch (opcode)
	{
		/* ARP request? */
		case ARP_REQUEST:
			{
				clog(info, CMARK, DBG_SYSTEM, "F:%s: ARP_REQUEST", __FUNCTION__);
				/* 
					Request who-has 10.10.10.10 tell 10.10.10.11, length 28 
					Now we sould iterate current iface
					1. only self mac address or broadcast
					2. check target IP addr (10.10.10.11) is setup on interface ifr.
					3. Xmit arp reply on same interface 
				*/
				if (is_eth_broadcast(&eth->dest) || memcmp(&eth->dest.addr, &ifr->hwaddr.addr, ETH_HWADDR_LEN) == 0)
				{
					// We should handle this packet
					// now we should iterate all ip addresses on this interface
					clog(info, CMARK, DBG_SYSTEM, "F:%s: ACCEPT ARP_REQUEST", __FUNCTION__);
					if (mux_ipconf_addr_is_exist(ifr->ip, dipaddr))
					{
						char *pkt_out;
						pkt_out = malloc(0xFF);
						struct etharp_hdr *arp_reply = (struct etharp_hdr *) (pkt_out + SIZEOF_ETH_HDR);
						memset(arp_reply, 0xAA, SIZEOF_ETH_HDR + SIZEOF_ETHARP_HDR);

						arp_reply->opcode    = ntohs(ARP_REPLY);
						arp_reply->hwtype    = ntohs(IANA_HWTYPE_ETHERNET);
						arp_reply->hwlen     = ETH_HWADDR_LEN;
						arp_reply->protolen  = 4;
						arp_reply->proto     = ntohs(ETHTYPE_IP);
						arp_reply->sipaddr   = dipaddr;
//						memset(&arp_reply->shwaddr, 0xFF,ETH_HWADDR_LEN );
						memcpy(&arp_reply->shwaddr, &ifr->hwaddr.addr, ETH_HWADDR_LEN);
						etharp_output(ifr, pkt_out, &ifr->hwaddr, &eth->src, ETHTYPE_ARP);                      
						clog(info, CMARK, DBG_SYSTEM, "F:%s:  ARP_REQUEST RESOLVED LOCAL!", __FUNCTION__);
						free(pkt_out);
					}



					// Broadcast or for us.

					//etharp_output(rif, pkt, &ifr->hwaddr, &eth->src, ETHTYPE_ARP);
				}
			}
			break;
		case ARP_REPLY:
			{
				struct in_addr ipaddr;
				ipaddr.s_addr = arp->sipaddr;
				if (is_eth_broadcast(&eth->dest) || memcmp(&eth->dest.addr, &ifr->hwaddr.addr, ETH_HWADDR_LEN) == 0)
				{
					clog(info, CMARK, DBG_SYSTEM, "F:%s: ARP_REPLY _FOR US_ %s is on %02x:%02x:%02x:%02x:%02x:%02x",
						 __FUNCTION__, inet_ntoa(ipaddr),
						 arp->shwaddr.addr[0], arp->shwaddr.addr[1], arp->shwaddr.addr[2],
						 arp->shwaddr.addr[3], arp->shwaddr.addr[4], arp->shwaddr.addr[5]);


					ArpRecord *upd = mux_arpdb_search(arp->sipaddr);
					if (upd)
					{
						clog(info, CMARK, DBG_SYSTEM, "F:%s: Update arp record!", __FUNCTION__);
						mux_arpdb_update(&arp->shwaddr, arp->sipaddr, ifr, ETHARP_STATE_STABLE);

					}
					else
					{
						clog(info, CMARK, DBG_SYSTEM, "F:%s: Can't find arp record. Ignore", __FUNCTION__);
					}
				}
				else
				{
					clog(info, CMARK, DBG_SYSTEM, "F:%s: ARP_REPLY _NOT FOR US_", __FUNCTION__);
				}


			}
			break;
		default:
			{
				clog(info, CMARK, DBG_SYSTEM, "F:%s: ARP unknown opcode type: %u", __FUNCTION__, htons(arp->opcode));
			}
			break;

	}


}





/* Bridge fdb service function. Should call every 1 sec. */
void mux_arpdb_service_1s()
{
	struct arp_record *s;
	struct in_addr ipaddr;
//	printf("-----------------------------------------------\n");
	for (s=arpdb; s != NULL; s = (struct arpdb_record *)(s->hh.next))
	{
//		printf("| %p state=%i  ctime=%u  |\n", s, s->state, s->ctime);

		if (s->state != ETHARP_STATE_STATIC)
		{
			s->ctime++;
			if ((s->ctime >= ARP_MAXAGE) ||
				((s->state == ETHARP_STATE_PENDING)  &&  (s->ctime >= ARP_MAXPENDING)))
			{
				/* pending or stable entry has become old! */
				/*
				clog(info, CMARK, DBG_SYSTEM, "F:%s: expired record %s. %02x:%02x:%02x:%02x:%02x:%02x <--> %s  IfName: '%s' %p ",
					 s->state >= ETHARP_STATE_STABLE ? "stable" : "pending",
					 s->arp.addr[0], s->arp.addr[1], s->arp.addr[2], 
					 s->arp.addr[3], s->arp.addr[4], s->arp.addr[5], 
					 inet_ntoa(ipaddr), s->ifr->name,  s);
				*/
				ipaddr.s_addr  = s->ip.s_addr;
				printf("s->ifr->name=%p\n", s->ifr->name);
				clog(info, CMARK, DBG_SYSTEM, "F:%s: %s: expired record %s. %02x:%02x:%02x:%02x:%02x:%02x <--> %s",
					 __FUNCTION__, s->ifr->name,
					 s->state >= ETHARP_STATE_STABLE ? "STABLE" : "PENDING",
					 s->arp.addr[0], s->arp.addr[1], s->arp.addr[2], 
					 s->arp.addr[3], s->arp.addr[4], s->arp.addr[5], 
					 inet_ntoa(ipaddr));
				HASH_DEL(arpdb, s);
				free(s);
			}
			else
				if (s->state == ETHARP_STATE_STABLE_REREQUESTING_1)
			{
				/*  Don't send more than one request every 2 seconds. */
				s->state = ETHARP_STATE_STABLE_REREQUESTING_2;
			}
			else
				if (s->state == ETHARP_STATE_STABLE_REREQUESTING_2)
			{
				/* Reset state to stable, so that the next transmitted packet will
					re-send an ARP request. */
				s->state = ETHARP_STATE_STABLE;
			}
			else
				if (s->state == ETHARP_STATE_PENDING)
			{
				MuxIPCfg *rfrom = mux_ipconf_match4(s->ifr->ip, s->ip);
				if (rfrom)
				{
					struct eth_addr addmac;
					memset(&addmac.addr, 0xFF, ETH_HWADDR_LEN); 
					// kickstart first resolve try;
					mux_arpdb_send_request(s->ifr, rfrom->ipaddr, s->ip.s_addr);
				}
				else
				{
					clog(info, CMARK, DBG_SYSTEM, "F:%s: Can't find ARP resolution source address", __FUNCTION__);

				}

				/* 	still pending, resend an ARP query */
//							etharp_request(arp_table[i].netif, &arp_table[i].ipaddr);
			}

		}
	}

	timer_add_ms("ARP_SERVICE", 1000, mux_arpdb_service_1s, NULL);
	return;
}

