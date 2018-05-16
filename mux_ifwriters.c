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
// TUN/TAP
#include <linux/if.h>
#include <linux/if_tun.h>


#include <net/if_arp.h>

#include "mux.h"

extern CLOG_INFO *info;

// int                (*tx_callback) (MUXCTX *ctx, void *rif, char *pkt, u_int32_t len);		 // TX function


void mux_ifwrite_tap(MUXCTX *ctx, MuxIf_t *wif, char *pkt, u_int32_t len)
{
	if (!wif || !pkt || !len || wif->type != MUX_IF_TAP )
		return;

	int wres = write(wif->id, pkt, len);
	if (wres <= 0)
		clog(info, CERROR, DBG_TUNTAP, "F:%s: %s: Write packet error '%s'", __FUNCTION__, wif->name, strerror(errno));
	return;
}

void mux_ifwrite_ring(MUXCTX *ctx, MuxIf_t *wif, char *pkt, u_int32_t len)
{
	int rc;
	if (!wif || !pkt || !len || wif->type != MUX_IF_RING)
		return;
	printf("RING WRITE %i bytes\n", len);
	hex_dump(pkt, len);

	rc = pfring_send(wif->ring, (char *)pkt, len, 1);
	//if (rc < 0)
                                                

}

/* Write to to all bridge group */
void mux_ifwrite_bridge(MUXCTX *ctx, MuxIf_t *in_if, char *pkt, u_int32_t len)
{
	MuxIf_t *w_if = NULL;
	u_int32_t brid = in_if->br_id;



	mux_ifmgr_itreset();
	

	w_if = mux_ifmgr_it_next();
	if (w_if)
	do
	{
		if (w_if->br_id == in_if->br_id && w_if != in_if)
		{
			printf("SEND TO %s\n", w_if->name);
			w_if->tx_callback(ctx, w_if, pkt, len);
		}
		w_if = mux_ifmgr_it_next();
	} while(w_if);

}
