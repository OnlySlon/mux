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

extern CLOG_INFO *info;

#define BR_FDB_TIMEOUT_SEC  (60*5) /* 5 minutes FDB timeout */



struct fdb_record *fdb = NULL;

/* 6 -> 4 byte hash. Little hack. */
inline u_int32_t mux_brfdb_hash(struct eth_addr *ehdr)
{
	return mycrc32(ehdr->addr, ETH_HWADDR_LEN);
}


void mux_brfdb_add(struct eth_addr *ehdr, MuxIf_t *ifr, u_int32_t br_id)
{
	struct fdb_record *s;

	int hash = (int) mux_brfdb_hash(ehdr);


	HASH_FIND_INT(fdb, &hash, s);  /* id already in the hash? */
	if (s == NULL)
	{
		
		s = (struct fdb_record *) malloc(sizeof(struct fdb_record));
		s->id  = hash;
		s->ttl = BR_FDB_TIMEOUT_SEC;
		s->br_id = br_id;
		s->iface = ifr;
		memcpy(s->eaddr.addr, ehdr->addr, ETH_HWADDR_LEN);
		clog(info, CMARK, DBG_SYSTEM, "F:%s: Create %02x:%02x:%02x:%02x:%02x:%02x IfName: '%s' Key:%04X %p",
		__FUNCTION__, ehdr->addr[0], ehdr->addr[1], ehdr->addr[2], ehdr->addr[3], ehdr->addr[4], ehdr->addr[5], ifr->name, hash,  s); 
		HASH_ADD_INT(fdb, id, s); 
	}

	return;
}


struct fdb_record *mux_brfdb_lookup(struct eth_addr *ehdr)
{
	struct fdb_record *s;
	int key = (int) mux_brfdb_hash(ehdr);
	HASH_FIND_INT(fdb, &key, s );
	if (s)
		return s;
	else
		return NULL;
} 


/* Bridge fdb service function. Should call every 1 sec. */
void mux_brfdb_service_1s()
{
	struct fdb_record *s;

	for (s=fdb; s != NULL; s = (struct fdb_record *)(s->hh.next))
	{
		s->ttl--;
		if (s->ttl == 0)
		{
			clog(info, CMARK, DBG_SYSTEM, "F:%s: Record #0x%08X expired.", __FUNCTION__, s->id);   
			HASH_DEL(fdb, s);
		}
	}
	return;
}


