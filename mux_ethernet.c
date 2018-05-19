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


/* Ethernet output function. VLAN's not supported*/
int etharp_output(MuxIf_t *netif, char *pkt,
					const struct eth_addr * src, 
					const struct eth_addr * dst,
					u_int16_t eth_type) 
{

	struct eth_hdr *ethhdr;
	u_int16_t eth_type_be = htons(eth_type);

	ethhdr = (struct eth_hdr *)(pkt);
	ethhdr->type = eth_type_be;
	memcpy(&ethhdr->dest.addr, dst->addr, ETH_HWADDR_LEN);
	memcpy(&ethhdr->src.addr,  src->addr, ETH_HWADDR_LEN);
	netif->tx_callback(NULL, netif, pkt, 60);
//					    (SIZEOF_ETH_HDR + SIZEOF_ETHARP_HDR));
	return 0;
}


int ethernet_output_ip(MuxIf_t *netif, 
					   char      *payload,
					   u_int32_t len,
					   u_int8_t  payload_proto)
{
	struct eth_hdr *eth = (struct eth_hdr *) (payload - IP_HLEN - SIZEOF_ETH_HDR);
	struct ip_hdr  *ip  = (struct ip_hdr *) (payload - IP_HLEN);
	u_int32_t arp_resolve_addr = 0;

	// Fill eth header
	eth->type = htons(ETHTYPE_IP);

	if (netif->flags & MUX_IF_F_GWDISCOVERY)
	{
		// Use different mechanism for send packet
		MuxGwDiscovery *gwd = mux_gwdiscovery_get(netif);
		if (!gwd)
		{
			clog(info, CWARN, DBG_SYSTEM, "F:%s: %s: GWDiscovery for interface '%s' not ready. Drop this packet", __FUNCTION__, netif->name);
			return 0;
		}
		clog(info, CWARN, DBG_SYSTEM, "F:%s: Use GWDiscovery mechanism for send packet from if: %s", __FUNCTION__, netif->name);
		// GWDiscovery mechanism is ready
		memcpy(&eth->src.addr,  &gwd->cli_hw.addr, ETH_HWADDR_LEN);
		memcpy(&eth->dest.addr, &gwd->gw_hw.addr,  ETH_HWADDR_LEN);

		ip->_chksum = in_cksum((char *) ip, IP_HLEN);

		netif->tx_callback(NULL, netif, eth, (len +  IP_HLEN + SIZEOF_ETH_HDR));
		return 0;
	}


	memcpy(&eth->src.addr, &netif->hwaddr, ETH_HWADDR_LEN);


	// test dst address is local
	MuxIPCfg *gw;
	MuxIPCfg *ipcfg = mux_ipconf_match4(netif->ip, htonl(ip->dst));

	int local = 0;
	
	if (ipcfg)
	{ 
		// Address is local - > need resolve dst address
		arp_resolve_addr = ntohl(ip->dst);
		local = 1;
	} else
	{
		// Try get default gateway for this interface
		gw = mux_ipconf_gw4(netif->ip);
		if (!gw)
		{
			// can't get gateway for this interface. Just drop packet
			clog(info, CWARN, DBG_SYSTEM, "F:%s: %s: No default gateway. Drop packet.", __FUNCTION__, netif->name);
			return -1;
		}
		arp_resolve_addr = gw->gateway;
	}

	ArpRecord *ar = mux_arpdb_resolve(arp_resolve_addr, 
									  netif,
									  (char *) eth, 
									  (len +  IP_HLEN + SIZEOF_ETH_HDR));


	ip->_chksum = in_cksum((char *) ip, IP_HLEN);

	if (ar)
	{
		// dst arp resolved.
		memset(&eth->dest, 0xFF, ETH_HWADDR_LEN);

		netif->tx_callback(NULL, netif, eth, (len +  IP_HLEN + SIZEOF_ETH_HDR));
	} else
	{
		// arp not resolved. packet put to arpdb queues
		clog(info, CWARN, DBG_SYSTEM, "F:%s: %s: Can't resolve ARP!!!", __FUNCTION__, netif->name);
	}


	return 0;
}





