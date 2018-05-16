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

void segv_handler(int sig);

int test_1s()
{

	printf("!!!!!!!!!!!!   HELLO 1SEC test\n");
	timer_add_ms("test", 1000, test_1s, NULL);
}

int main(int argc, char *argv[])
{
	MUXCTX *ctx = NULL;

	ctx = (MUXCTX *) mux_ctx_init();
	
	info =  ctx->info;


	signal(SIGSEGV, segv_handler); 
	clog(ctx->info, CMARK, DBG_SYSTEM, "F:%s: HELLO WORLD", __FUNCTION__);

	mux_ifread_init();
	timers_init();


	MuxIf_t *testdev1; 
	MuxIf_t *testdev2; 
	MuxIf_t *testdev3; 
	testdev1 = mux_ifmgr_add(ctx, "RING_ETH1", MUX_IF_RING, "eth1");
	if (!testdev1)
	{
		printf("testdev1 FAIL\n");
		exit(-1);
	}

/*
	testdev2 = mux_ifmgr_add(ctx, "RING_ETH0", MUX_IF_RING, "eth0"); // eth0
	if (!testdev2)
	{
		printf("testdev2 FAIL\n");
		exit(-1);
	}
*/
	testdev3 = mux_ifmgr_add(ctx, "TAP_TAP0 ", MUX_IF_TAP, "tap0");
	if (!testdev3)
	{
		printf("testdev3 FAIL\n");
		exit(-1);
	}

	struct eth_addr eaddr;
	eaddr.addr[0] =  0x22;
	eaddr.addr[1] =  0x32;
	eaddr.addr[2] =  0x42;
	eaddr.addr[3] =  0x52;
	eaddr.addr[4] =  0x62;
	eaddr.addr[5] =  0x72;
	mux_brfdb_add(&eaddr, testdev3);


//	mux_ifmgr_setbridge(testdev1, 2);
//	mux_ifmgr_setbridge(testdev2, 2);
//	mux_ifmgr_setbridge(testdev3, 2);

	mux_ipconf_add(testdev1, inet_addr("10.10.10.20"), 24, inet_addr("0.0.0.0"));
//	printf(">>>>>>>>>>>> %p\n", testdev2->ip);


	//some_exec("ifconfig tap0 10.10.10.10 netmask 255.255.255.0");
	int it;
	MuxIf_t *fif;
	u_int32_t counter = 0;



	// Setup service timers
	timer_add_ms("ARP_SERVICE", 1000, mux_arpdb_service_1s, NULL);

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
			test_ip_send(testdev1);
		}
		timer_run();
		counter++;

	}

}
