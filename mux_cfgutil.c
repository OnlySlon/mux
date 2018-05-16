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


MUXCTX *mux_ctx_init()
{
	MUXCTX *ctx = NULL;
	ctx = (MUXCTX *) malloc(sizeof(MUXCTX));
	memset(ctx, 0, sizeof(MUXCTX));

	ctx->polldb = (struct pollfd *) malloc(32 * sizeof(struct pollfd));
	return ctx;
}
