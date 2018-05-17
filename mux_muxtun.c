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




// Multi-Tun side of tunnel
typedef struct mux_muxtun
{
	int                id;
	u_int16_t          id_global;
	u_int32_t          ip_remote;
	u_int32_t          ip_local;
	u_int32_t          group;
	u_int32_t          state;
	MuxTransport      *transport;
	UT_hash_handle     hh;    
} MuxMuxTun;



enum MuxMuxStates
{
	MUX_MUXSTATE_DOWN = 0,
	MUX_MUXSTATE_UP,
};

MuxTransport MuxTransportRAW;


MuxMuxTun *MuxTunDB = NULL;

/*
typedef struct MUX_IFCONF_PLUGIN_S
{
    u_int32_t  type;             // plugin type
    char      *name;         // plugin name
    char       bpf_filter[64];   // bpf filter for incoming traffic. Pass to plugin
    int        (*init_cb)  (void *priv);
    int        (*rx_cb)    (void *priv, char *pkt, u_int32_t len);
    int        (*get_info4)(void *priv, void *data, u_int32_t *len);
    
    void       *priv;            // private plugin data
} MuxIfConf_t;

*/





MuxMuxTun *mux_muxtun_new(MuxTransport *tr_type)
{
	static u_int16_t tunid = 0;
	MuxMuxTun *s = NULL;
	int key;
	
	// first generate  unique tunid
	do
	{
		tunid = (u_int16_t) (rand32() & 0xFFFF);
		key = (int) tunid;
		HASH_FIND_INT(MuxTunDB, &key, s);
	} while(s != NULL);

	// get random tunid on start

	s = (MuxMuxTun *) malloc(sizeof(MuxMuxTun));

	s->transport = mux_transport_init(tr_type);
	if (s == NULL)
		goto rollback;

	s->id          = (int) tunid;
	s->id_global   = tunid;


	
	return s;

	rollback:
	{
		if (s)
			free(s);
		return (MuxMuxTun *) NULL;
	}
}


void mux_muxtun_match(MuxMuxTun *muxdb, char *pkt)
{
	MuxMuxTun *s;
	struct in_addr ipaddr;
	struct ip_hdr *iphdr  =  (struct ip_hdr *) (pkt + SIZEOF_ETH_HDR);
	u_int32_t tr_offset   = 0;

	if (iphdr->_proto == IPPROTO_UDP)
	{
		tr_offset = SIZEOF_ETH_HDR + SIZEOF_IP_HDR + SIZEOF_UDP_HDR;
	} else
	{
		tr_offset = SIZEOF_ETH_HDR + SIZEOF_IP_HDR;
	}

	MuxProtoTun *hdr = (MuxProtoTun *) (pkt + tr_offset);


	int hash =  (int) ntohs(hdr->tun_id);

	HASH_FIND_INT(MuxTunDB, &hash, s); 
	if (s) 
	{
		// Tun with this ID exist
		// handle tun data

	} else
	{
		clog(info, CMARK, DBG_SYSTEM, "F:%s: Can't find MuxTun with id: 0x%04X", __FUNCTION__, ntohs(hdr->tun_id));

	}

	for (s = muxdb; s != NULL; s = (MuxMuxTun *)(s->hh.next))
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





