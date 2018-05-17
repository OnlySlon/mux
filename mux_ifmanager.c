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
#include <stdio.h>
#include <stdlib.h>
// TUN/TAP
#include <linux/if.h>
#include <linux/if_tun.h>


#include <net/if_arp.h>

#include "mux.h"

/*
	MUX hw interface manager
*/

extern CLOG_INFO *info;



void mux_ifread_ring(MUXCTX *ctx, MuxIf_t *rif);
void mux_ifread_tap(MUXCTX *ctx, MuxIf_t *rif);
void mux_ifwrite_ring(MUXCTX *ctx, MuxIf_t *wif, char *pkt, u_int32_t len);
void mux_ifwrite_tap(MUXCTX *ctx, MuxIf_t *wif, char *pkt, u_int32_t len);

MuxIf_t *ifdb = NULL;

#define MAX_PKT_LEN 1536

char *mux_ifmgr_tpe(int type)
{
	switch (type)
	{
		case MUX_IF_BRIDGE:
			return "MUX_IF_BRIDGE";
		case MUX_IF_TUN:
			return "MUX_IF_TUN";
		case MUX_IF_TAP:
			return "MUX_IF_TAP";
		case MUX_IF_RAW:
			return "MUX_IF_RAW";
		case MUX_IF_RING:
			return "MUX_IF_RING";
		default:
			return "MUX_IF___UNKNOWN___";
	}
	return "NEVER_HAPPENDS";
}

/*
#define MUX_IF_F_MASTER       (1 << 0)
#define MUX_IF_F_DISABLED     (1 << 1)
#define MUX_IF_F_GWDISCOVERY  (1 << 2)
#define MUX_IF_F_CLIENT       (1 << 3)
#define MUX_IF_F_SRVDISCOVERY (1 << 4)
*/

char *mux_ifmgr_flags2string(u_int32_t f)
{
	static char str[512];
	memset(str, 0, 512);
	if (f & MUX_IF_F_MASTER)
		strcat(str, "MASTER, ");
	else
		strcat(str, "SLAVE, ");

	if (f & MUX_IF_F_DISABLED)
		strcat(str, "DISABLED, ");
	else
		strcat(str, "ENABLED, ");

	if (f & MUX_IF_F_GWDISCOVERY)    strcat(str, "GWDISCOVERY, ");
	if (f & MUX_IF_F_CLIENT)         strcat(str, "CLIENT, ");
	if (f & MUX_IF_F_SRVDISCOVERY)   strcat(str, "MUX_IF_F_SRVDISCOVERY, ");


	if (strlen(str) > 0) str[strlen(str) - 2] = 0;

	return str;
}




/* debug service */
void mux_ifmgr_service_10s()
{
	MuxIf_t *s = NULL;
	char brid[10];
	clog(info, CMARK, DBG_TUNTAP, "--------------------------------------------------------'", __FUNCTION__);
	for (s = ifdb; s != NULL; s = (struct MuxIf_t *)(s->hh.next))
	{

		snprintf(brid, 9, "Br%d", s->br_id);
		clog(info, CERROR, DBG_TUNTAP, "%-12s  Type: %-15s  Bridge: %s", 
			 s->name, 
			 mux_ifmgr_tpe(s->type),
			 s->br_id ? brid : "-Disabled-"); 

		clog(info, CERROR, DBG_TUNTAP, "%-12s  Flags: %s", 
				"", mux_ifmgr_flags2string(s->flags));

		clog(info, CERROR, DBG_TUNTAP, "      ");


	}
	clog(info, CMARK, DBG_TUNTAP, "--------------------------------------------------------'", __FUNCTION__);

	timer_add_ms("IFDEBUG_SERVICE", 5000, mux_ifmgr_service_10s, NULL);
}



int tun_alloc(char *dev, int flags) 
{
	struct ifreq ifr;
	int fd, err;

	if ( (fd = open("/dev/net/tun", O_RDWR)) < 0 )
	{
		clog(info, CERROR, DBG_TUNTAP, "F:%s: Can't open /dev/net/tun: '%s'", __FUNCTION__, strerror(errno));
		return fd;
	}
	memset(&ifr, 0, sizeof(ifr));
	ifr.ifr_flags = flags;
	if (*dev)
		strncpy(ifr.ifr_name, dev, IFNAMSIZ);

	if ( (err = ioctl(fd, TUNSETIFF, (void *)&ifr)) < 0 )
	{
		clog(info, CERROR, DBG_TUNTAP, "F:%s: ioctl(TUNSETIFF): '%s'", __FUNCTION__, strerror(errno));
		close(fd);
		return err;
	}
//	strcpy(dev, ifr.ifr_name);
	return fd;
}


void polldb_rebuild(MUXCTX *ctx)
{
	MuxIf_t *s;
	ctx->fds_allocated = 0;
	for (s=ifdb; s != NULL; s= (MuxIf_t *)(s->hh.next))
	{
		ctx->polldb[ctx->fds_allocated].fd      = s->id;
		ctx->polldb[ctx->fds_allocated].events  = POLLIN | POLLERR;
		ctx->polldb[ctx->fds_allocated].revents = 0;
		ctx->fds_allocated++;
	}
	clog(ctx->info, CMARK, DBG_SYSTEM, "F:%s: Rebuilding pollfd DB done. Records: %i", __FUNCTION__, ctx->fds_allocated);
	return;
}


MuxIf_t *mux_ifmgr_add(MUXCTX *ctx, char *ifname, u_int32_t tpe, void *params, u_int32_t flags)
{
	static if_idx = 0;
	MuxIf_t *muxif = (MuxIf_t *) malloc(sizeof(MuxIf_t));
	memset(muxif, 0, sizeof(sizeof(MuxIf_t)));
	strncpy(muxif->name, ifname, MUX_IF_NAME_LEN-1);
	muxif->type = tpe;
	muxif->tx_callback = NULL;
	muxif->ip          = NULL;
	switch (muxif->type)
	{
		case MUX_IF_RING:
			{
				/*
					-------=====  RING ADD PROCEDURE =====-------
				*/
				pfring *ring;
				char *hwname = (char *) params;
				clog(ctx->info, CMARK, DBG_SYSTEM, "F:%s: Adding IfName: '%s', HWIfName: '%s' Type: MUX_IF_RING", 
					 __FUNCTION__, ifname, hwname);

				// Try up iface
				{
					char exec_me[512];
					sprintf(exec_me, "ifconfig %s up", hwname);
					some_exec(exec_me);
				}

				// Open device
				if ((ring = pfring_open(hwname, 
										MAX_PKT_LEN, 
										PF_RING_PROMISC | PF_RING_LONG_HEADER)) == NULL)
				{
					clog(info, CFATAL, DBG_SYSTEM, "F:%s: pfring_open error for %s [%s]", 
						 __FUNCTION__, ifname, strerror(errno));
					goto rollback;
				}

				// Configure device
				pfring_set_application_name(ring, "MUX");
				pfring_set_direction(ring , rx_and_tx_direction);
				pfring_set_socket_mode(ring, send_and_recv_mode);
				pfring_get_bound_device_ifindex(ring, &muxif->ring_ifindex);
				pfring_set_poll_watermark(ring , 1);
				if (pfring_enable_ring(ring) != 0)
				{
					clog(info, CFATAL, DBG_SYSTEM, "F:%s: Unable enabling ring ", __FUNCTION__);
					pfring_close(ring);
					goto rollback;
				}

				strncpy(muxif->hwname, hwname, MUX_IF_NAME_LEN-1);
				muxif->ring        = ring;
				muxif->id          = pfring_get_selectable_fd(ring);
				muxif->rx_callback = mux_ifread_ring;
				muxif->tx_callback = mux_ifwrite_ring;
				pfring_sync_indexes_with_kernel(ring);

				// Determine HW addr
				{
					char hwaddr[6];
					if (iface_get_hwaddr(hwname, hwaddr) == 0)
					{
						memcpy(muxif->hwaddr.addr, hwaddr, ETH_HWADDR_LEN);
						clog(info, CMARK, DBG_SYSTEM, "F:%s: HW addr of iface '%s'  %02x:%02x:%02x:%02x:%02x:%02x",
							 __FUNCTION__,  hwname, (unsigned char) hwaddr[0], (unsigned char) hwaddr[1], (unsigned char) hwaddr[2], 
							 (unsigned char) hwaddr[3], (unsigned char) hwaddr[4], (unsigned char) hwaddr[5]);
					} else
					{
						clog(info, CFATAL, DBG_SYSTEM, "F:%s: Unable Detect HWAddr of iface: '%s'", __FUNCTION__, hwname);
					}
				}
			}
			break;
		case MUX_IF_TAP:
			{
				pfring *ring;
				int tap_fd;
				char *hwname = (char *) params;
				struct ifreq ifr;

				clog(ctx->info, CMARK, DBG_SYSTEM, "F:%s: Adding IfName: '%s', HWIfName: '%s' Type: MUX_IF_TAP", 
					 __FUNCTION__, ifname, hwname);

				if ((tap_fd = tun_alloc(hwname, IFF_TAP | IFF_NO_PI)) < 0 )
				{
					clog(ctx->info, CERROR, DBG_SYSTEM, "F:%s: Error connecting to tun/tap interface %s", __FUNCTION__, hwname);
					goto rollback;
				}
				// Set FD NONBLOCK
				fcntl(tap_fd, F_SETFL, fcntl(tap_fd, F_GETFL, 0) | O_NONBLOCK);

				ifr.ifr_hwaddr.sa_data[0] = 0x12;
				ifr.ifr_hwaddr.sa_data[1] = 0x13;
				ifr.ifr_hwaddr.sa_data[2] = 0x14;
				ifr.ifr_hwaddr.sa_data[3] = 0x15;
				ifr.ifr_hwaddr.sa_data[4] = 0x16;
				ifr.ifr_hwaddr.sa_data[5] = 0x17;

				memcpy(muxif->hwaddr.addr, ifr.ifr_hwaddr.sa_data, 6);
				int s = socket(AF_INET, SOCK_DGRAM, 0);
				strcpy(ifr.ifr_name, hwname);
				ifr.ifr_hwaddr.sa_family = ARPHRD_ETHER;
				ioctl(s, SIOCSIFHWADDR, &ifr);
				close(s);

				muxif->id          = tap_fd;
				muxif->rx_callback = mux_ifread_tap;
				muxif->tx_callback = mux_ifwrite_tap;
			
				strncpy(muxif->hwname, hwname, MUX_IF_NAME_LEN-1);
				// up interface
				{
					char exec_me[512];
					sprintf(exec_me, "ifconfig %s up", hwname);
					some_exec(exec_me);
				}
			}
			break;
		default:
			{
				clog(ctx->info, CERROR, DBG_SYSTEM, "F:%s: IfName: '%s'. Unknown type of interface (%u).", __FUNCTION__, ifname, (unsigned) muxif->type);
				return NULL;
			}
	}

	muxif->flags = flags;
	HASH_ADD_INT(ifdb, id, muxif);
	polldb_rebuild(ctx);
	if_idx++;
	clog(ctx->info, CMARK, DBG_SYSTEM, "F:%s: Adding IfName: '%s' DONE! Key=%i", __FUNCTION__, ifname, muxif->id);
	return muxif;

	rollback:
	clog(ctx->info, CERROR, DBG_SYSTEM, "F:%s: Adding IfName: '%s' ROLLBACK!", __FUNCTION__, ifname);
	if (muxif)
		free(muxif);
	return NULL;
}

/*
	Enable interface work as bridge & setup bridge_id
*/
int mux_ifmgr_setbridge(MuxIf_t *muxif, u_int32_t br_id)
{
	if (br_id)
	{
		clog(info, CMARK, DBG_SYSTEM, "F:%s: SETBRIDGE IfName: '%s' BridgeID: %u", __FUNCTION__, muxif->name, br_id);
		muxif->is_bridge = 1;
		muxif->br_id     = br_id;
	} else
	{
		clog(info, CMARK, DBG_SYSTEM, "F:%s: Disable bridge IfName: '%s'", __FUNCTION__, muxif->name, br_id);
		muxif->is_bridge = 0;
		muxif->br_id     = 0;

	}
	return 0;
}

/* delete interface from manager */
int *mux_ifmgr_del(MuxIf_t *muxif)
{
	// FIX-ME
	// Find device
	// remove procedure depends on ->type
	if (!muxif)
		return -1;

	clog(info, CMARK, DBG_SYSTEM, "F:%s: Delete IfName: '%s' Type: '%s'", 
		 __FUNCTION__, muxif->name, mux_ifmgr_tpe(muxif->type));

	switch (muxif->type)
	{
		case MUX_IF_RING:
			{
				pfring_close(muxif->ring);
			}
			break;
		case MUX_IF_TAP:
			{
				char exec_str[512];
				sprintf(exec_str, "ifconfig %s down", muxif->hwname);
				some_exec(exec_str);
				close(muxif->fd);
			}
			break;
		default:
			{
				clog(info, CERROR, DBG_SYSTEM, "F:%s: Unsupported delete procedure IfName: '%s' Type: '%s'", 
					 __FUNCTION__, muxif->name, mux_ifmgr_tpe(muxif->type));
				return -1;
			}
	}

	HASH_DEL(ifdb, muxif);
	free(muxif);
	return 0;
}


MuxIf_t *mux_ifmgr_find(int key)
{
	MuxIf_t *muxif = NULL;
	HASH_FIND_INT(ifdb, &key, muxif);
	return muxif;
}


MuxIf_t *it_if = NULL;

void mux_ifmgr_itreset()
{
	it_if = ifdb;
}

MuxIf_t *mux_ifmgr_it_next()
{
	MuxIf_t *from = it_if;

	if (!from) return NULL;
	it_if = (MuxIf_t *) from->hh.next;

	return  from;

}



