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

#include "mux.h"


typedef struct POLLDB_S
{
	int               id;	   // fd
	int               type;	   // type of record 
	void             *ptr;	   // record pointer
	UT_hash_handle    hh;
} PollDB;

void polldb_rebuild()
{
	uplink_t *s;
	fds_allocated = fds_offset;
	for (s=uplinkdb; s != NULL; s= (uplink_t *)(s->hh.next))
	{
		polldb[fds_allocated].fd      = s->id;
		polldb[fds_allocated].events  = POLLIN | POLLERR;
		polldb[fds_allocated].revents = 0;
		fds_allocated++;
	}
}
