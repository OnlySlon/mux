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

/* 
    Server - just bridge
    Instances
    	- Clients
    	  - Intercept Nets
    	  - Configuration
    - assocs Intercept_nets <--> client instance
 
*/
 
 
CLOG_INFO *info;

int main(int argc, char *argv[])
{
	printf("Hello world!\n");
}
