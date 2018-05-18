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


#define MAX_INLINE_PKTS 10

u_char *_buffer;
u_char *buffer;


void mux_ifread_init()
{
	_buffer = malloc(0xFFF); // 4095
	buffer = _buffer + 0xFF; // for pass some data at head of packet

}

/* >>>>>>>>>>>>> main entrance of Ethernet packets <<<<<<<<<<<<<<<<< */
void mux_ether_input(MUXCTX *ctx, MuxIf_t *rif, char *pkt, u_int32_t len)
{
	struct eth_hdr *ehdr = (struct eth_hdr *) pkt;

	unip_debug_packet(pkt, 1, rif->name);


	// Should we stop handle packet after some situations ???? FIX-ME
	if (ntohs(ehdr->type) == ETHTYPE_ARP)
		etharp_input(rif, pkt, len);

	if (rif->is_bridge)
	{
		MuxIf_t *brif = NULL;
		struct fdb_record *fr;

		mux_gwdiscovery_sniff(rif, pkt, len);


		// store source address
		mux_brfdb_add(&ehdr->src, rif);

		fr = mux_brfdb_lookup(&ehdr->dest);

		if (fr)
		{
			brif = fr->iface;
			if (brif->tx_callback)
			{
				brif->tx_callback(ctx, brif, pkt, len);
			} else
			{
				printf("NO TX CALLBACK! name=%s\n", brif->name);
			}
			return;
		}

		mux_ifwrite_bridge(ctx, rif, pkt, len);
		
	} else
	{
//		printf("NOT BRIDGE\n");
	}
}

void mux_ifread_ring(MUXCTX *ctx, MuxIf_t *rif)
{
	int inl = 0; // Inline packets
	struct pfring_pkthdr hdr;

	
	// disable incorrect type
	if (rif->type != MUX_IF_RING) return;
	
	while (pfring_is_pkt_available(rif->ring) && (inl <= MAX_INLINE_PKTS))
	{
		inl++;
		if (pfring_recv(rif->ring, &buffer, 0, &hdr, 0) > 0) // 0 - wait for packet
		{
			if (hdr.extended_hdr.rx_direction)
			{
				rif->stats.rx_packets++;
				rif->stats.rx_bytes += (u_int64_t) hdr.caplen;
				mux_ether_input(ctx, rif, (char *) buffer, hdr.caplen);
			} 
		}  else
		{
			rif->stats.rx_errors++;
		}
	}
}


void mux_ifread_tap(MUXCTX *ctx, MuxIf_t *rif)
{

	if (rif->type != MUX_IF_TAP) return;

	int pkt_len = read(rif->id, buffer, 1500);

	if (unlikely(pkt_len <= 0))
	{
		rif->stats.rx_errors++;
		return;
	}
	rif->stats.rx_packets++;
	rif->stats.rx_bytes += (u_int64_t) pkt_len;

	mux_ether_input(ctx, rif, (char *) buffer, pkt_len);

}


