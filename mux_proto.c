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
char *UUID;


u_int32_t mode;



u_int32_t mux_proto_ashmhdr(char *space,
	                        u_int32_t type,
							u_int32_t flags,
							MuxMuxTun *tun,
							MuxDevInstance *dev)
{
	u_int32_t         offset = sizeof(MuxProtoHdr);
	MuxProtoHdr      *hdr;
	MuxProtoCrc      *crc;
	MuxProtoSeqInfo  *seqinfo;
	hdr = (MuxProtoHdr *) space;


	hdr->flags = flags;
	hdr->tunid = htons((u_int16_t) tun);

	if (flags & MUX_PROTO_F_HAVECRC)
	{
		crc  = (MuxProtoCrc *) (((char *)  hdr) + offset);
		crc->crc  = 0;
		offset   += sizeof(MuxProtoCrc);
	}

	if (flags & MUX_PROTO_F_HAVESEQINFO)
	{
		// Add seqinfo header
		seqinfo  =  (MuxProtoSeqInfo *) (((char *)  hdr) + offset);		
		seqinfo->seq_global = 0;
		seqinfo->seq_local  = 0;
		offset  += sizeof(MuxProtoSeqInfo);
	}


	switch (type)
	{
		case MUX_PROTO_TPE_DATA:
			{
				// FIXME
			}
			break;
		case MUX_PROTO_TPE_HANDSHAKE:
			{
				MuxProtoHandshakeT *hshake =  (MuxProtoHandshakeT *) (((char *)  hdr) + offset);
				if (mode == MUX_CLIENT)
				{
					memcpy(hshake->UUID, UUID, MUX_UUID_LEN);
					hshake->stage = MUX_PROTO_SSHAKE_REQUEST;
					hshake->tunid = 0;
				} else
				{
					
					if (dev)
					{
						memcpy(hshake->UUID, dev->UUID, MUX_UUID_LEN);
						hshake->payload_size = 0;
						hshake->stage        = 0;
						hshake->tunid        = 0;
					} else
					{
						memset(hshake->UUID, 0, MUX_UUID_LEN);
					}					
				}
				offset += sizeof(MuxProtoHandshakeT);
			}
			break;

	}

	return offset;

}


u_int32_t mux_proto_deasm(char *pkt, u_int32_t len)
{
	u_int32_t    tr_offset = 0;
	u_int32_t    offset = sizeof(MuxProtoHdr);
	MuxProtoHdr *hdr = (MuxProtoHdr *) (pkt + tr_offset);


	if (hdr->flags & MUX_PROTO_F_HAVECRC)
	{
		MuxProtoCrc *crc  = (MuxProtoCrc *) (((char *)  hdr) + offset);
		u_int16_t pkt_crc = crc->crc;
		offset += sizeof(MuxProtoCrc);
	}

	if (hdr->flags & MUX_PROTO_F_HAVESEQINFO)
	{
		MuxProtoSeqInfo *seqinfo  =  (MuxProtoSeqInfo *) (((char *)  hdr) + offset);		
		offset += sizeof(MuxProtoCrc);
	}


	switch(hdr->type)
	{
		case MUX_PROTO_TPE_DATA:
			{
				u_int32_t tunid = (u_int32_t) ntohs(hdr->tunid);
				MuxMuxTun *tun = mux_muxtun_match(tunid);
				if (tun)
				{
					// Need forward this packet
				} else
				{
					// just ignore this packet
				}
			}
			break;
		case MUX_PROTO_TPE_HANDSHAKE:
			{
				MuxProtoHandshakeT *hshake =  (MuxProtoHandshakeT *) (((char *)  hdr) + offset);
				if (mode == MUX_SERVER)
				{
					 // need create device instance 
				} else
				{
					// create tun instances 
				}

			}
			break;

	}
}

