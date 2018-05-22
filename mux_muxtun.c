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





MuxMuxTun *mux_muxtun_new(MuxTransport        *tr_type,
						  MuxProtoHandshakeT  *hshake, 
						  MuxDevInstance      *device)
{
	static u_int16_t tunid = 0;
	MuxMuxTun *s = NULL;
	int key;
	

	// if got hshake args - check for same ifname
	if (hshake && device)
	{
		for (s=MuxTunDB; s != NULL; s = (MuxMuxTun *)(s->hh.next))
		{
			if (s->device == device && 
				strncmp(hshake->ifname, s->ifname_remote, MUX_IFNAME_MAXLEN - 1) == 0 && 
				strncmp(hshake->UUID, s->device->UUID, MUX_UUID_LEN) == 0 )
			{
				clog(info, CMARK, DBG_SYSTEM, "F:%s: MuxTun with same UUID and devname already exist", __FUNCTION__);
				// This record already exist. 
				return s;
			}
			
		}
	}

	if (hshake->stage == MUX_PROTO_SSHAKE_REQUEST)
	{
		// first generate  unique tunid >>>>>>>>>> SERVER SIDE
		do
		{
			tunid = (u_int16_t) (rand32() & 0xFFFF);
			key = (int) tunid;
			HASH_FIND_INT(MuxTunDB, &key, s);
		} while(s != NULL);
	} else
	{
		// Client side -> accept tun id from server
		tunid = ntohs(hshake->tunid);
	}


	// get random tunid on start

	s = (MuxMuxTun *) malloc(sizeof(MuxMuxTun));

	s->tr = mux_transport_init(tr_type);
	if (s == NULL)
		goto rollback;

	s->id          = (int) tunid;
	s->id_global   = tunid;

	clog(info, CMARK, DBG_SYSTEM, "F:%s: New MuxTun (%p) generated (TunID=%u)", __FUNCTION__, s, (unsigned) tunid);
	
	HASH_ADD_INT(MuxTunDB, id, s);
	return s;

	rollback:
	{
		if (s)
			free(s);
		return (MuxMuxTun *) NULL;
	}
}


MuxMuxTun * mux_muxtun_match(u_int32_t tunid)
{
	MuxMuxTun *s;
	int hash = (int) tunid;

	HASH_FIND_INT(MuxTunDB, &hash, s); 
	if (s) 
	{
		// Tun with this ID exist
		// handle tun data
		return s;

	} else
	{
		clog(info, CMARK, DBG_SYSTEM, "F:%s: Can't find MuxTun with id: 0x%04X", __FUNCTION__, tunid);
		return (MuxMuxTun *) NULL;

	}
/*
	for (s = muxdb; s != NULL; s = (MuxMuxTun *)(s->hh.next))
	{

    }
*/
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





