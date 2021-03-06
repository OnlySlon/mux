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
typedef struct MUXIPCFG_S
{
	// ipv4 only
	u_int32_t          ipaddr;
	u_int8_t           mask;
	u_int32_t          gateway;
	UT_hash_handle     hh;
} MuxIPCfg;

*/


void mux_ipconf_service_1s()
{

	MuxIf_t *sif;

	// start interface iterator

	mux_ifmgr_itreset(); // reset iterator

	while((sif = mux_ifmgr_it_next()) != NULL)
	{		
		if (sif->ip) // have some kind of ip configuration
		{
			if (sif->ip->gateway != 0) // ip gateway is exist
			{
					ArpRecord *s = mux_arpdb_search(sif->ip->gateway);
					if (s == NULL) // ARP record for gateway !NOT exist 
					{
						clog(info, CMARK, DBG_SYSTEM, "F:%s: %s: Start GW ARP discovery!", __FUNCTION__, sif->name);

						mux_arpdb_resolve(sif->ip->gateway, sif, NULL, 0);

					}
			}
		}
	}


	timer_add_ms("IPCONF_SERVICE", 1000, mux_ipconf_service_1s, NULL);
}






MuxIPCfg * mux_ipconf_add(MuxIf_t  *sif, 
						  u_int32_t ipaddr,
						  u_int32_t mask,
						  u_int32_t gateway,
						  u_int32_t flags)
{
    MuxIPCfg *s;   
	MuxIPCfg *ipdb;
	int success = 0;
	mask = mask & 0xFF;

	int hash = (int) ipaddr;
	
	HASH_FIND_INT(ipdb, &hash, s);  /* id already in the hash? */
	if (s == NULL)
	{
		s = (struct MuxIPCfg *) malloc(sizeof(MuxIPCfg));
		memset(s, 0, sizeof(MuxIPCfg));
		s->ipaddr  = ipaddr;
		s->gateway = gateway;
		s->mask    = mask;
		s->flags   = flags;
		HASH_ADD_INT(sif->ip, ipaddr, s);
		success  = 1;
	} else
	{
		success  = 0;
	}


	{
		struct in_addr ips;
		char ip[32];
		char gw[32];
		ips.s_addr = ipaddr;
		strcpy(ip, inet_ntoa(ips));
		ips.s_addr = gateway;
		strcpy(gw, inet_ntoa(ips));


		clog(info, CMARK, DBG_SYSTEM, "F:%s:  IfName: '%s' Addr: %s/%u GW: %s Primary: %s Result: %s DB: %p Key=0x%04X%",
			 __FUNCTION__, sif->name, ip, mask, gw, (!(s->flags & MUX_IPCFG_F_SECONDARY) ? "YES" : "NO"),
			 success ? "SUCCESS" : "FAILED!!!", sif->ip, hash); 
	}


	return success ? s : NULL;
}

void mux_ipconf_free(MuxIPCfg *ipdb)
{
	MuxIPCfg *s;

	for (s = ipdb; s != NULL; s = (MuxIPCfg *)(s->hh.next))
		HASH_DEL(ipdb, s);

	return;
}

void mux_ipconf_del(MuxIPCfg *ipdb, MuxIPCfg *todel)
{
	MuxIPCfg *s;

	// WTF?!
	for (s = ipdb; s != NULL; s = (MuxIPCfg *)(s->hh.next)) 
		if (s == todel)
			HASH_DEL(ipdb, s);
	return;
}


void mux_ipconf_debug(MuxIPCfg   *ipdb)
{
	MuxIPCfg *s;
	struct in_addr ips;

	for (s = ipdb; s != NULL; s = (MuxIPCfg *)(s->hh.next))
	{
		ips.s_addr = s->ipaddr;
		clog(info, CMARK, DBG_SYSTEM, "F:%s:  IPDB: %p   Here: %s",
			 __FUNCTION__, ipdb, inet_ntoa(ips)); 
	}

}



// Host byte order
MuxIPCfg *mux_ipconf_match4(MuxIPCfg   *ipdb,
							u_int32_t   ipaddr)
{
	MuxIPCfg *s;
	struct in_addr ips;
	struct in_addr ipf;
	ipf.s_addr = ipaddr;
	char str[512];
	sprintf(str, "%s", inet_ntoa(ipf));
//	clog(info, CMARK, DBG_SYSTEM, "F:%s:  >>>MATCH-4 IPDB: %p Try resolve %s", __FUNCTION__, ipdb, str);
	for (s = ipdb; s != NULL; s = (MuxIPCfg *)(s->hh.next))
	{
		ips.s_addr = s->ipaddr;
//		clog(info, CMARK, DBG_SYSTEM, "F:%s:  IPDB: %p Look:%s   Here: %s/%u 0x%08X M:0x%08X", 
//			 __FUNCTION__, ipdb, str, inet_ntoa(ips), s->mask, ((htonl(s->ipaddr) ^ htonl(ipaddr)) ) & htonl(0xFFFFFFFFu << (32 - s->mask)));  
/*
		clog(info, CMARK, DBG_SYSTEM, "F:%s:  IPDB: %p Look:%s   Here: %s/%u 0x%08X",
			 __FUNCTION__, ipdb, str, inet_ntoa(ips), s->mask, ((s->ipaddr ^ ipaddr) & htonl(0xFFFFFFFFu << (32 - s->mask)))); 

*/

//		if ((ipaddr & s->mask) ==  (s->ipaddr & s->mask))
		if (!((s->ipaddr ^ ipaddr) & htonl(0xFFFFFFFFu << (32 - s->mask))))
		{
//			clog(info, CMARK, DBG_SYSTEM, "F:%s: MATCHED!", __FUNCTION__);
			return s;
		}
	}
//	clog(info, CMARK, DBG_SYSTEM, "F:%s: NOT MATCHED!", __FUNCTION__);
	return (MuxIPCfg *) NULL;
}


// Get packet source for transfer IP packets Fuck the RFC FIX ME
MuxIPCfg *mux_ipconf_src(MuxIPCfg   *ipdb, u_int32_t dst)
{
	MuxIPCfg *s;
	s = mux_ipconf_match4(ipdb, dst);
	if (s)
		return s;
	if (ipdb)
		return ipdb;
	return 0;
}


MuxIPCfg *mux_ipconf_gw4(MuxIPCfg   *ipdb)
{
	MuxIPCfg *s;

	for (s = ipdb; s != NULL; s = (MuxIPCfg *)(s->hh.next))
	{
		if (s->gateway != 0)
		{
			return s;
		}
	}
	return (MuxIPCfg *) NULL;
}




MuxIPCfg *mux_ipconf_addr_is_exist(MuxIPCfg   *ipdb,
							      u_int32_t   ipaddr)
{
	MuxIPCfg *s;
	struct in_addr ips;
	ips.s_addr =  ipaddr;

	int hash = (int) ipaddr;
	HASH_FIND_INT(ipdb, &hash, s); 
	clog(info, CMARK, DBG_SYSTEM, "F:%s: Lookup IPADDR: %s res=%p DB: %p key=0x%04X%", __FUNCTION__, inet_ntoa(ips), s, ipdb, hash);
	mux_ipconf_debug(ipdb);
	return s;
}




