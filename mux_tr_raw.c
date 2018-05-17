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




typedef struct MUX_RawPrivate
{
	u_int32_t  hz;
} RawPrivate;


int mux_tr_raw_init(void *priv)
{
	RawPrivate *p = malloc(sizeof(RawPrivate));
	memset(p, 0, sizeof(RawPrivate));
	p->hz = 0;
	return 0;
}

int mux_tr_raw_destroy(void *priv)
{
	if (priv)
		free(priv);
	return 0;
}

int mux_tr_raw_send(void *priv, char *pkt, int len)
{
	return 0;
}

int mux_tr_raw_recv(void *priv, char *pkt)
{
	return 0;
}


MuxTransport MuxTransportRAW =
{
		MUX_TR_RAW,
        (char *) "RAW",
        mux_tr_raw_init,
        mux_tr_raw_send,
        mux_tr_raw_recv,
		mux_tr_raw_ctrl,
	    mux_tr_raw_destroy,
        (SIZEOF_ETH_HDR + IP_HLEN), // just ethenet +ip 
        (void *) NULL
};

