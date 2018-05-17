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



void mux_discovery_service(void *ptr)
{
	MuxIf_t *dif;

	// start interface iterator

	mux_ifmgr_itreset(); // reset iterator

	while((dif = mux_ifmgr_it_next()) != NULL)
	{
		if (dif->flags & MUX_IF_F_SRVDISCOVERY)
		{

		}
	}



}
