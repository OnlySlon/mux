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


CLOG_INFO *info;

// Private struct
struct mux_ifconf_mnlp
{
	int test;
};

int *ifcfg_manual_init(MuxIfConf_t *ic)
{
    struct mux_ifconf_mnlp *p;
    p = (struct mux_ifconf_mnlp *)  malloc(sizeof(struct mux_ifconf_mnlp));
    memset(p, 0, sizeof(struct mux_ifconf_mnlp));
    ic->priv = (void *) p;

}


/*


struct MUX_IFCONF_PLUGIN_S MUX_IFCONF_MNL = 
{
    MUX_IFCONF_MANUAL,
    (char *) "MANUAL",
    (char *) NULL,
    ifcfg_manual_init,    // init callback
    ifcfg_manual_rx,      // rx_callback
    (void *) NULL         // Private data

}
*/
