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
#include <signal.h>

#include "mux.h"

CLOG_INFO *info;


enum MuxDiscoveryState {
    MUX_GWDISCOVERY_PRETENDER = 0,
    MUX_GWDISCOVERY_COMMITED,
	MUX_GWDISCOVERY_RECHECK,
};

#define MUX_GWDISCOVERY_TTL            60          // 60 sec.
#define MUX_GWDISCOVERY_TTL_RECHECK    10 

typedef struct mux_gwdiscovery_info
{
	int                id;     // key in this case - 48to32 bit hash of gateway mac address
	u_int32_t          state;
	struct eth_addr    gw_hw;
	struct eth_addr    cli_hw;
	struct in_addr     cli_ip;
	struct in_addr     gw_ip;  // We don't know gateway ip address on gwdiscovery mode
	u_int32_t          ttl;
	MuxIf_t          *iface;
	UT_hash_handle     hh;
} MuxDiscovery;

MuxDiscovery *DiscoveryDB = NULL;


/* 48 to 32 bit hash function */
u_int32_t mux_mac_hash(u_char *mac)
{
        return mycrc32(mac, ETH_HWADDR_LEN);
}


/* gwdiscovery service for delete expired records */
void mux_gwdiscovery_service_1s()
{
	MuxDiscovery *s;
	for (s = DiscoveryDB; s != NULL; s = (struct MuxDiscovery *)(s->hh.next))
	{
		if (s->ttl > 0)
			s->ttl--;

		if (s->ttl <= MUX_GWDISCOVERY_TTL_RECHECK)
		{
			s->state = MUX_GWDISCOVERY_RECHECK;
			clog(info, CMARK, DBG_SYSTEM, "F:%s: %s: Discovery record %p Need to recheck. GWHW: %02x:%02x:%02x:%02x:%02x:%02x", __FUNCTION__, s->iface->name, s,
				 s->gw_hw.addr[0], s->gw_hw.addr[1], s->gw_hw.addr[2], s->gw_hw.addr[3], s->gw_hw.addr[4], s->gw_hw.addr[5]);

		}

		if (s->ttl == 0)
		{
			clog(info, CMARK, DBG_SYSTEM, "F:%s: %s: Discovery record %p expired! GWHW: %02x:%02x:%02x:%02x:%02x:%02x", __FUNCTION__, s->iface->name, s,
				 s->gw_hw.addr[0], s->gw_hw.addr[1], s->gw_hw.addr[2], s->gw_hw.addr[3], s->gw_hw.addr[4], s->gw_hw.addr[5]);
			HASH_DEL(DiscoveryDB, s);
			free(s);
		}
	}
	timer_add_ms("GWDISCOVERY_SERVICE", 1000, mux_gwdiscovery_service_1s, NULL);
}


/* 
    Sniff only INCOMING traffic on ifaces
    Sniff on CLIENT & MASTER links for determine client and gateway params
 
*/ 
void mux_gwdiscovery_sniff(MuxIf_t  *iface, char *pkt, u_int32_t len)
{
	// Skip sniff if interface in non-gwdiscovery state
	if (!(iface->flags & MUX_IF_F_GWDISCOVERY))
		return;

	struct eth_hdr    *eth = (struct eth_hdr *) pkt;
	struct ip_hdr     *ip =  (struct ip_hdr *) (pkt + SIZEOF_ETH_HDR);
	MuxDiscovery *s;

	clog(info, CMARK, DBG_SYSTEM, "F:%s: %s: PKT IN", __FUNCTION__, iface->name);

	if (iface->flags & MUX_IF_F_CLIENT)
	{
		// >>>>>>>>>>>>>>  Sniff on client link side. >>>>>>>>>> GW_PARAMS: DST
		int hash = mux_mac_hash(&eth->dest.addr);
		clog(info, CMARK, DBG_SYSTEM, "F:%s: %s: CLIENT LINK hash=0x%08X", __FUNCTION__, iface->name, (unsigned) hash);

		HASH_FIND_INT(DiscoveryDB, &hash, s);
		if (s == NULL)
		{
			// Record not exist. Create new. 
			s = (MuxDiscovery *) malloc(sizeof(MuxDiscovery));
			memset(s, 0, sizeof(MuxDiscovery));
			s->cli_ip.s_addr = ip->src;
			s->gw_ip.s_addr  = 0; // always caluse we cat get it from gwdiscovery
			memcpy(&s->gw_hw.addr,  &eth->dest.addr,  ETH_HWADDR_LEN);
			memcpy(&s->cli_hw.addr, &eth->src.addr,  ETH_HWADDR_LEN);
			s->ttl    = MUX_GWDISCOVERY_TTL;
			s->iface  = iface;
			s->state  = MUX_GWDISCOVERY_PRETENDER;
			s->id     = hash;

			clog(info, CMARK, DBG_SYSTEM, "F:%s: %s: New pretender GWHW: %02x:%02x:%02x:%02x:%02x:%02x CLIHW: %02x:%02x:%02x:%02x:%02x:%02x CliIP: %s", 
				 __FUNCTION__, iface->name, eth->dest.addr[0], eth->dest.addr[1], eth->dest.addr[2],
				 eth->dest.addr[3], eth->dest.addr[4], eth->dest.addr[5],
				 eth->src.addr[0], eth->src.addr[1], eth->src.addr[2],
				 eth->src.addr[3], eth->src.addr[4], eth->src.addr[5],
				inet_ntoa(s->cli_ip)); 
			HASH_ADD_INT(DiscoveryDB, id, s);
			// Ok PRETENDER now in database;
		} else
		{
			// this record already exist
		}
	} else
	{
		clog(info, CMARK, DBG_SYSTEM, "F:%s: %s: SERVER LINK", __FUNCTION__, iface->name);
		// Sniff on master link side
		int hash = mux_mac_hash(&eth->src.addr);
		HASH_FIND_INT(DiscoveryDB, &hash, s);
		if (s)
		{
			// We got a PRETENDER??? >>>>>>>>>> GW_PARAMS: SRC
			if (s->state == MUX_GWDISCOVERY_PRETENDER ||
				s->state == MUX_GWDISCOVERY_RECHECK)
			{
				hex_dump(&s->cli_hw.addr, ETH_HWADDR_LEN);
				hex_dump(&eth->dest.addr, ETH_HWADDR_LEN);
				// Ok we got record in MUX_GWDISCOVERY_PRETENDER state. 
				if (s->cli_ip.s_addr == ip->dst &&
					memcmp(&s->cli_hw.addr, &eth->dest.addr, ETH_HWADDR_LEN) == 0 && 
					memcmp(&s->gw_hw.addr,  &eth->src.addr,  ETH_HWADDR_LEN) == 0)
				{
					clog(info, CMARK, DBG_SYSTEM, "F:%s: %s: GW_MASTER <<-- MUXBOX Macthed!! GW switch to MUX_GWDISCOVERY_COMMITED", __FUNCTION__, iface->name);
					s->state = MUX_GWDISCOVERY_COMMITED;
					s->ttl   = MUX_GWDISCOVERY_TTL;
				} else
				{
					clog(info, CMARK, DBG_SYSTEM, "F:%s: %s: GW_MASTER <<-- MUXBOX  NOT MATCHED!!!! %08X %08X", __FUNCTION__, iface->name, s->cli_ip.s_addr, ip->dst);
				}
			}
		}
	}
		
}

