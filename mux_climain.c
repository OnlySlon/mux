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

void mux_arpdb_service_1s();
void mux_gwdiscovery_service_1s();
void mux_ipconf_service_1s();
void mux_ifmgr_service_10s();
void segv_handler(int sig);

/* 
    main client  test
    - 3 interface
                             MUX-BOX
         192.168.0.1/24 ------------------
GW1<-> E0----MASTER---> | <---bridge---> | E2 <----------> client (CLI_MAC, CLIIP)
                        |                |
GW2<-> E1----SLAVE----> |                |
         10.0.0.1/24    ------------------
 
 
 E0 and E2 work as bridge
 
 GW1 - GW1_IP, GW1_MAC
 GW2 - GW2_IP, GW2_MAC 
 MB  - MB_E0_IP, MB_E0_MAC (E0)
 MB  - MB_E1_IP, MB_E1_MAC (E1)
 MB  - MB_E2_IP, MB_E2_MAC (E2)
 CL  - CLI_IP,   CLI_MAC
 
 
 - Master - Discovery Mode only
 - Slsve  - Get 1st ip address from settings
 
*/ 

extern MuxTransport MuxTransportRAW;

void mux_clihandshake_service_1s(void *ptr)
{
	static int init = 1;
	static   MuxTransport *tr = NULL;

	if (init == 1)
	{
		init = 0;
		MuxIf_t *dif;
		clog(info, CMARK, DBG_SYSTEM, "F:%s: Init handshake transport....", __FUNCTION__);
		tr = mux_transport_init(&MuxTransportRAW);
		mux_ifmgr_itreset();

		while((dif = mux_ifmgr_it_next()) != NULL)
		{			
			if (dif->flags & MUX_IF_F_MASTER ) // do it only on MASTER interface
			{
				char *tmp = malloc(0xFFF);
				char *pkt = tmp + SIZEOF_ETH_HDR + SIZEOF_IP_HDR;
				sprintf(pkt, "HELLO HANDSHAKE HELLO HANDSHAKE HELLO HANDSHAKE HELLO HANDSHAKE HELLO HANDSHAKE HELLO HANDSHAKE HELLO HANDSHAKE HELLO HANDSHAKE ");
				tr->tr_ctl(tr->priv, MUX_TR_OP_SETIF, dif, sizeof(MuxIf_t));
				tr->tr_sendto(tr->priv, inet_addr("5.5.5.5"), pkt, strlen(pkt));
			}
		}

		// set interface
		


	}
	// send handhsake packets to server

	exit(0);

	timer_add_ms("CLIHANDSHAKE_SERVICE", 1000, mux_clihandshake_service_1s, NULL );
}


int main(int argc, char *argv[])
{
	MUXCTX *ctx = NULL;

	ctx = (MUXCTX *) mux_ctx_init();
	
	info =  ctx->info;

#define BRIDGE_ID_MAIN 1

//	signal(SIGSEGV, segv_handler); 
	

	mux_uuid();

	mux_ifread_init();
	timers_init();


	MuxIf_t *masterdev; 
	MuxIf_t *slavedev1; 
	MuxIf_t *clientdev;
	MuxIf_t *intbridge;
	
	// Setup master device. Enable IP discovery
	masterdev = mux_ifmgr_add(ctx, "MASTER_ETH1", MUX_IF_RING, "master0", MUX_IF_F_MASTER | MUX_IF_F_GWDISCOVERY);
	if (!masterdev)
	{
		clog(info, CMARK, DBG_SYSTEM, "F:%s: Masterdev create fail", __FUNCTION__);
		exit(-1);
	}

	slavedev1 = mux_ifmgr_add(ctx, "SLAVE1_ETH2", MUX_IF_RING, "slave0", MUX_IF_F_SLAVE);
	if (!slavedev1)
	{
		clog(info, CMARK, DBG_SYSTEM, "F:%s: slavedev1 create fail", __FUNCTION__);
		exit(-1);
	}

	intbridge = mux_ifmgr_add(ctx, "TAP_TAP0 ", MUX_IF_TAP, "tap0");
	if (!intbridge)
	{
		clog(info, CMARK, DBG_SYSTEM, "F:%s: intbridge  create fail", __FUNCTION__);
		exit(-1);

	}

	clientdev = mux_ifmgr_add(ctx, "CLIENT_ETH3", MUX_IF_RING, "client0", MUX_IF_F_SLAVE | MUX_IF_F_CLIENT | MUX_IF_F_GWDISCOVERY);
	if (!clientdev)
	{
		clog(info, CMARK, DBG_SYSTEM, "F:%s: clientdev create fail", __FUNCTION__);
		exit(-1);
	}


	


	//  Add bridge interface
	struct eth_addr eaddr;
	eaddr.addr[0] =  0x22;
	eaddr.addr[1] =  0x32;
	eaddr.addr[2] =  0x42;
	eaddr.addr[3] =  0x52;
	eaddr.addr[4] =  0x62;
	eaddr.addr[5] =  0x72;

	mux_brfdb_add(&eaddr, masterdev, BRIDGE_ID_MAIN);

	// Add masterdev and clientdev to main bridge
	mux_ifmgr_setbridge(masterdev, BRIDGE_ID_MAIN);
	mux_ifmgr_setbridge(clientdev, BRIDGE_ID_MAIN);
	mux_ifmgr_setbridge(intbridge, BRIDGE_ID_MAIN);
	

	mux_ipconf_add(slavedev1, inet_addr("11.11.11.11"), 24, inet_addr("11.11.11.1"));
//	printf(">>>>>>>>>>>> %p\n", testdev2->ip);


	some_exec("ifconfig tap0 10.10.10.10 netmask 255.255.255.0");
	int it;
	MuxIf_t *fif;
	u_int32_t counter = 0;



	// Setup service timers
	// ARP service timer 1 sec.
	timer_add_ms("ARP_SERVICE", 1000, mux_arpdb_service_1s, NULL);
	timer_add_ms("GWDISCOVERY_SERVICE", 1000, mux_gwdiscovery_service_1s, NULL);
	timer_add_ms("IPCONF_SERVICE", 100, mux_ipconf_service_1s, NULL);
	timer_add_ms("IFDEBUG_SERVICE", 1000, mux_ifmgr_service_10s, NULL);
	timer_add_ms("CLIHANDSHAKE_SERVICE", 1000, mux_clihandshake_service_1s, NULL );


	while(1)
	{
		poll(ctx->polldb, ctx->fds_allocated, 500);
		for(it=0;it<=ctx->fds_allocated;it++)
		{
			if (ctx->polldb[it].revents & POLLIN)
			{
				fif = mux_ifmgr_find(ctx->polldb[it].fd);
				if (fif)
				{
					if (fif->rx_callback) fif->rx_callback(ctx, fif);
				} else
				{
					printf("FD #%i NOT FOUND!!!!\n", ctx->polldb[it].fd);
				}
			}
		}

		if ((counter%10) == 0) 
		{
			//test_ip_send(slavedev1);
		}
		timer_run();
		counter++;

	}

}

