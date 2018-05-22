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

extern CLOG_INFO *info;




typedef struct MUX_RawPrivate
{
	u_int32_t        hz;
	MuxIf_t         *tif;
	MuxTransport    *self_ptr;
	int              gwhw_isset;
	struct eth_addr  gwhw;
	int              remote_isset;
	u_int32_t        remote;
} RawPrivate;


int mux_tr_raw_init(void *self_ptr)
{	
	
	RawPrivate *p = malloc(sizeof(RawPrivate));
	clog(info, CMARK, DBG_SYSTEM, "F:%s:  >> INIT RAW TRANSPORT << (%p)", __FUNCTION__, p); 
	memset(p, 0, sizeof(RawPrivate));
	p->self_ptr     = self_ptr;
	p->gwhw_isset   = 0;
	p->remote_isset = 0;
	p->hz           = 0;
	return p;
}

int mux_tr_raw_destroy(void *priv)
{
	if (priv)
		free(priv);
	return 0;
}

int mux_tr_raw_send(void *priv, char *pkt, int len)
{
	RawPrivate *p = (RawPrivate *) priv;
	if (p->gwhw_isset && p->remote_isset)
	{
		printf("DIRTY HACK!!!!!\n\n\n\n");
		struct eth_hdr *eth = (struct eth_hdr *) (pkt - IP_HLEN - SIZEOF_ETH_HDR);
		u_int32_t src = 0x05050505;
		u_int32_t dst = p->remote;
		memcpy(&eth->dest.addr, &p->gwhw.addr, ETH_HWADDR_LEN);
		mux_ip_output_if(p->tif, src,  dst, MUX_PROTONUM, pkt,  len, MUX_TR_F_DSTHW_EXTST);  
		return 0;
	}
	return -1;
}

int mux_tr_raw_sendto(void *priv, u_int32_t dstip,  char *pkt, int len)
{
	RawPrivate *p = (RawPrivate *) priv;

	clog(info, CMARK, DBG_SYSTEM, "F:%s: TR: %p YEHAAA!!! len=%i", __FUNCTION__, p, len); 
	

	char src_str[512];
	char dst_str[512];

	sprintf(src_str, "0.0.0.0");
	sprintf(dst_str, "10.10.10.30");
	sprintf(dst_str, "10.10.10.11");
	struct in_addr src;
	struct in_addr dst;
	src.s_addr = 0;//  inet_addr(src_str);
	dst.s_addr = dstip;
//	printf("IP PL LEN=%i\n", strlen(payload));


//	clog(info, CMARK, DBG_SYSTEM, "F:%s: %s: TESTPACKET %s --> %s", __FUNCTION__, txif->name, src_str, dst_str);

	mux_ip_output_if(p->tif, src.s_addr, dst.s_addr, MUX_PROTONUM, pkt,  len, 0);  


}

int mux_tr_raw_recv(void *priv, char *pkt)
{
	return 0;
}

int mux_tr_raw_ctrl(void *priv, u_int32_t op, char *op_val, int oplen)
{
	RawPrivate *rp = (RawPrivate *) priv;
	switch(op)
	{
		case MUX_TR_OP_SETIF:
			{
				MuxIf_t *sif = op_val;
				clog(info, CMARK, DBG_SYSTEM, "F:%s: TR:%p OP: MUX_TR_OP_SETIF. IF: %s ", __FUNCTION__, rp,  sif->name); 
				rp->tif = op_val;
				return 0;
			}
			break;
		case MUX_TR_OP_SETGWHW:
			{
				if (oplen == ETH_HWADDR_LEN && op_val)
				{
					memcpy(&rp->gwhw.addr, op_val, ETH_HWADDR_LEN);
					clog(info, CMARK, DBG_SYSTEM, "F:%s: TR:%p OP: MUX_TR_OP_SETGWHW.  GwHW: %02x:%02x:%02x:%02x:%02x:%02x", __FUNCTION__, rp, 
						  rp->gwhw.addr[0], rp->gwhw.addr[1], rp->gwhw.addr[2], rp->gwhw.addr[3], rp->gwhw.addr[4], rp->gwhw.addr[5]); 
					rp->gwhw_isset = 1;
					
				}
				return 0;
			}
			break;
		case MUX_TR_OP_SETREMOTE: 
			{
				if (oplen == 4 && op_val)
				{
					rp->remote_isset = 1;
					memcpy(&rp->remote, op_val, 4);
					struct in_addr addr;
					addr.s_addr = rp->remote;
					clog(info, CMARK, DBG_SYSTEM, "F:%s: TR:%p OP: MUX_TR_OP_SETREMOTE. IP: %s ", __FUNCTION__, rp, inet_ntoa(addr)); 
					

				}
				return 0;
			}
			break;
		default:
			{
				return -1; 
			}
			break;
	}
}

MuxTransport MuxTransportRAW =
{
		MUX_TR_RAW,
        (char *) "RAW",
        mux_tr_raw_init,
        mux_tr_raw_send,
		mux_tr_raw_sendto,
        mux_tr_raw_recv,
		mux_tr_raw_ctrl,
	    mux_tr_raw_destroy,
        (SIZEOF_ETH_HDR + IP_HLEN), // just ethenet +ip 
        (void *) NULL
};

