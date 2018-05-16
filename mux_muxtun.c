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



typedef struct Mux_Proto
{
	u_int16_t          magic;		   // MuxIP proto magic								   //
	u_int16_t          tun_id;		   // Mux  global TunID
	u_int16_t          seq_gl;		   // global tun seq
	u_int16_t          seq_tun;		   // local tun seq
	u_int8_t           flags;
} MuxProtoTun;


#defui


// Multi-Tun side of tunnel
typedef struct mux_muxtun
{
	u_int32_t          id_global;
	u_int32_t          ip_remote;
	u_int32_t          ip_local;
	u_int32_t          group;
	u_int32_t          state;
	UT_hash_handle     hh;
} MuxMuxTun;



enum MuxMuxStates
{
	MUX_MUXSTATE_DOWN = 0,
	MUX_MUXSTATE_UP,
};



void mux_muxtun_match(MuxMuxTun *muxdb, char *pkt)
{
	MuxMuxTun *s;
	struct in_addr ipaddr;
	MuxProtoTun *hdr = (MuxProtoTun *) (pkt + 0);

	int hash =  (int) ntohs(hdr->tun_id);


	HASH_FIND_INT(muxdb, &hash, s); 
	if (s) 
	{
		// Tun with this ID exist
		// handle tun data

	} else
	{
		clog(info, CMARK, DBG_SYSTEM, "F:%s: Can't find MuxTun with id: 0x%04X", __FUNCTION__, ntohs(hdr->tun_id));

	}

	for (s=muxdb; s != NULL; s = (MuxMuxTun *)(s->hh.next))
	{

	}
}


void mux_pkt2tun()
{
	/*
	    1. match network - got tunId
	    2. find net state - balancing/copy
	    3. get tun
	    4. Xmit data
	*/
}





