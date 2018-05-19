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
    Skip values
    - ttl
    - ip options
    - tos
*/
int mux_ip_output_if(MuxIf_t     *txif,
					 u_int32_t    src,
					 u_int32_t    dst,
					 u_int8_t     proto,
					 char        *payload,
					 u_int32_t    len)
{
	struct ip_hdr *ip =  (struct ip_hdr *) (payload -  IP_HLEN);
	u_int16_t optlen = 0;

	if (src == 0)
	{
		MuxIPCfg *ipcfg = mux_ipconf_src(txif->ip, dst);
		if (ipcfg)
		{
//			printf("_________ipcfg=%p\n", ipcfg);
			src = htonl(ipcfg->ipaddr);
		}
	}

	ip->_v_hl    = 0x45;
	ip->_len     = ntohs(len);
	ip->_proto   = proto;
	ip->_tos     = IP_TOS_DEFAULT;
	ip->_ttl     = IP_TTL_DEFAULT;
	ip->_id      = htons(0xFFFF);
	ip->_offset  = 0;
	ip->dst      = htonl(dst);
	ip->src      = htonl(src);
	ip->_chksum  = 0;// in_cksum((char *) ip, IP_HLEN);

	// automatic set source address 
	if(src == 0)
	{
		if (txif->flags & MUX_IF_F_GWDISCOVERY)
		{
			MuxGwDiscovery *gwd = mux_gwdiscovery_get(txif);
			if (gwd)
				ip->src = gwd->cli_ip.s_addr;
		} else
		{
			if (txif->ip)
				ip->src = htonl(txif->ip->ipaddr);
		}
	}

	/*
		IP hdr is cooked. Now we should resolve destination HW addr
	*/

	ethernet_output_ip(txif, payload, len, proto);
}



void test_ip_send(MuxIf_t *txif)
{
	char *pkt = malloc(0xFFF);
	char *payload = pkt + MUX_IPPAYLOAD_OFF;
	sprintf(payload, "HELLO BABY!!!!! This is a house of the boy who was my best friend");
	

	char src_str[512];
	char dst_str[512];

	sprintf(src_str, "0.0.0.0");
	sprintf(dst_str, "10.10.10.30");
	sprintf(dst_str, "10.10.10.11");
	struct in_addr src;
	struct in_addr dst;
	src.s_addr = inet_addr(src_str);
	dst.s_addr = inet_addr(dst_str);
//	printf("IP PL LEN=%i\n", strlen(payload));


	clog(info, CMARK, DBG_SYSTEM, "F:%s: %s: TESTPACKET %s --> %s", __FUNCTION__, txif->name, src_str, dst_str);

	mux_ip_output_if(txif,src.s_addr ,dst.s_addr, IPPROTO_UDP, payload, strlen(payload));
}
