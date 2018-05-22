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

u_int32_t mux_proto_deasm(MuxIf_t *rif, char *pkt, u_int32_t len);
extern MuxTransport MuxTransportRAW;

int mux_proto_handler(MuxIf_t *rif, char *pkt, u_int32_t len)
{
	struct eth_hdr *eth     = (struct eth_hdr *) (pkt);
	struct ip_hdr  *ip      = (struct ip_hdr *) (pkt + SIZEOF_ETH_HDR);
	char           *payload = (pkt + SIZEOF_ETH_HDR + SIZEOF_IP_HDR);

	printf(">>>>>>>>>>>>>>>>>>>>>> PROTO HANDLER!!!!! %u\n", ip->_proto);
	if (ip->_proto == MUX_PROTONUM)
	{
		hex_dump(pkt, len);

		printf("Match proto!\n");


//		mux_proto_deasm(rif, payload, len - (SIZEOF_ETH_HDR + SIZEOF_IP_HDR), ip);
		mux_proto_deasm(rif, pkt, len);

//		exit(0);
		return MUX_PACKET_DROP; 
	}

	return MUX_PACKET_PASS;
}



u_int32_t mux_proto_ashmhdr(char *space,
	                        u_int32_t type,
							u_int32_t flags,
							MuxMuxTun *tun,
							MuxDevInstance *dev,
							MuxIf_t        *ifdev)
{
	u_int32_t         offset = sizeof(MuxProtoHdr);
	MuxProtoHdr      *hdr;
	MuxProtoCrc      *crc;
	MuxProtoSeqInfo  *seqinfo;
	hdr = (MuxProtoHdr *) space;


	hdr->flags = flags;
	hdr->tunid = htons((u_int16_t) tun);
	hdr->type  = (u_int8_t) type;

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
					// Client -> send self UUID
					memcpy(hshake->UUID, UUID, MUX_UUID_LEN);
					if (ifdev)
						strncpy(hshake->ifname, ifdev->name, MUX_IFNAME_MAXLEN -1);
					hshake->stage = MUX_PROTO_SSHAKE_REQUEST;
					hshake->tunid = 0;
				} else
				{
					// Server -> send server unique TunID
					
					if (dev)
					{
						clog(info, CMARK, DBG_SYSTEM, "F:%s: ASM HSHAKE REPLY....", __FUNCTION__);
						memcpy(hshake->UUID, dev->UUID, MUX_UUID_LEN);
						strncpy(hshake->ifname, ifdev->name, MUX_IFNAME_MAXLEN - 1);
						hshake->payload_size = 0;
						hshake->stage        = MUX_PROTO_SSHAKE_REPLY;
						hshake->tunid        = (u_int16_t) htons(tun->id);
					}					
				}
				offset += sizeof(MuxProtoHandshakeT);
			}
			break;

	}

	return offset;

}


u_int32_t mux_proto_deasm(MuxIf_t *rif, char *pkt, u_int32_t len)
{
	u_int32_t       tr_offset = SIZEOF_ETH_HDR + SIZEOF_IP_HDR;
	u_int32_t       offset    = sizeof(MuxProtoHdr);  // ETH + IP + MuxHdr
	struct ip_hdr  *ip        = (struct ip_hdr *) (pkt + SIZEOF_ETH_HDR);
	struct eth_hdr *eth       = (struct eth_hdr *) (pkt);
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
				clog(info, CMARK, DBG_SYSTEM, "F:%s: GOT MUX_PROTO_TPE_DATA: tunId: %u", __FUNCTION__, tunid);
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
				clog(info, CMARK, DBG_SYSTEM, "F:%s: GOT Hansdshake from: %s Stage=%s", __FUNCTION__, hshake->UUID, hshake->stage == MUX_PROTO_SSHAKE_REPLY ? "Reply" : "Request");
				hex_dump(hshake,  sizeof(MuxProtoHandshakeT));
				if (mode == MUX_CLIENT)
				{
					// Create client-side muxTun instance




				} else
				{
					// Server mode
					// create device  instances 
					MuxDevInstance *device = mux_muxdev_new(hshake);
					if (device)
					{
						// got device. new or already existed.
						MuxMuxTun *muxtun = mux_muxtun_new(&MuxTransportRAW, hshake, device);
						if (muxtun)
						{
							// Send back reply
							char *tmp = malloc(0xFFF);
							char *pkt = tmp + SIZEOF_ETH_HDR + SIZEOF_IP_HDR; 
							u_int32_t len = mux_proto_ashmhdr(pkt, MUX_PROTO_TPE_HANDSHAKE, MUX_PROTO_F_NOFLAGS, muxtun, device, rif);
							u_int32_t remote = ntohl(ip->src);

							len += SIZEOF_ETH_HDR + SIZEOF_IP_HDR;
							muxtun->tr->tr_ctl(muxtun->tr->priv, MUX_TR_OP_SETIF,     rif, sizeof(MuxIf_t));           // Link transport to incoming interface
							muxtun->tr->tr_ctl(muxtun->tr->priv, MUX_TR_OP_SETGWHW,   &eth->src.addr, ETH_HWADDR_LEN); // Link gateway to this HW address
							muxtun->tr->tr_ctl(muxtun->tr->priv, MUX_TR_OP_SETREMOTE, &remote, 4);
							muxtun->tr->tr_send(muxtun->tr->priv, pkt, len);

							// Link MuxTun with device
							muxtun->device = device;


						}

					}


				}

			}
			break;

	}
}

