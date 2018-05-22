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


/*

typedef struct mux_devinstance
{
	int                id;
	char               UUID[MUX_UUID_LEN];
	u_int32_t          stage;
	u_int32_t          group_id;
	UT_hash_handle     hh; 
} MuxDevInstance;

*/


MuxDevInstance *MuxDevDb = NULL;




MuxDevInstance *mux_muxdev_new(MuxProtoHandshakeT *hshake)
{
	MuxDevInstance *s = NULL; 
	int key = mycrc32(hshake->UUID, MUX_UUID_LEN);


	clog(info, CMARK, DBG_SYSTEM, "F:%s: Try create New Device with UUID: '%s'", __FUNCTION__, hshake->UUID);

	HASH_FIND_INT(MuxDevDb, &key, s);
	if (s)
	{
		// device already exist
		clog(info, CWARN, DBG_SYSTEM, "F:%s: Device with same UUID already exist", __FUNCTION__, hshake->UUID);
		return s;
	}


	s = malloc(sizeof(MuxDevInstance));
	memset(s, 0, sizeof(MuxDevInstance));
	memcpy(s->UUID, hshake->UUID, MUX_UUID_LEN);
	s->id = key;
	s->group_id = ntohs(hshake->group_id);
	s->stage    = MUX_DEVSTAGE_INITIAL;



	clog(info, CMARK, DBG_SYSTEM, "F:%s: Create new device instance UUID: %s", __FUNCTION__, hshake->UUID); 
	HASH_ADD_INT(MuxDevDb, id, s);
	return s;
}
