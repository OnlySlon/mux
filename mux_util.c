#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <net/if.h>
#include <linux/if_tun.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <sys/time.h>
#include <errno.h>
#include <stdarg.h>
#include <poll.h>
#include <signal.h>
#include <time.h>
#include <execinfo.h>
#include <signal.h>

#include "mux.h"
extern CLOG_INFO *info;


#define CRC32C(c, d) (c = (c >> 8) ^ crc_c[(c ^ (d)) & 0xFF])

static const unsigned int crc_c[256] = {
	0x00000000, 0x77073096, 0xee0e612c, 0x990951ba,
	0x076dc419, 0x706af48f, 0xe963a535, 0x9e6495a3,
	0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988,
	0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91,
	0x1db71064, 0x6ab020f2, 0xf3b97148, 0x84be41de,
	0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
	0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec,
	0x14015c4f, 0x63066cd9, 0xfa0f3d63, 0x8d080df5,
	0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172,
	0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b,
	0x35b5a8fa, 0x42b2986c, 0xdbbbc9d6, 0xacbcf940,
	0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
	0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116,
	0x21b4f4b5, 0x56b3c423, 0xcfba9599, 0xb8bda50f,
	0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924,
	0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d,
	0x76dc4190, 0x01db7106, 0x98d220bc, 0xefd5102a,
	0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
	0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818,
	0x7f6a0dbb, 0x086d3d2d, 0x91646c97, 0xe6635c01,
	0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e,
	0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457,
	0x65b0d9c6, 0x12b7e950, 0x8bbeb8ea, 0xfcb9887c,
	0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
	0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2,
	0x4adfa541, 0x3dd895d7, 0xa4d1c46d, 0xd3d6f4fb,
	0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0,
	0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9,
	0x5005713c, 0x270241aa, 0xbe0b1010, 0xc90c2086,
	0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
	0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4,
	0x59b33d17, 0x2eb40d81, 0xb7bd5c3b, 0xc0ba6cad,
	0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a,
	0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683,
	0xe3630b12, 0x94643b84, 0x0d6d6a3e, 0x7a6a5aa8,
	0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
	0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe,
	0xf762575d, 0x806567cb, 0x196c3671, 0x6e6b06e7,
	0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc,
	0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5,
	0xd6d6a3e8, 0xa1d1937e, 0x38d8c2c4, 0x4fdff252,
	0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
	0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60,
	0xdf60efc3, 0xa867df55, 0x316e8eef, 0x4669be79,
	0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236,
	0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f,
	0xc5ba3bbe, 0xb2bd0b28, 0x2bb45a92, 0x5cb36a04,
	0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
	0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a,
	0x9c0906a9, 0xeb0e363f, 0x72076785, 0x05005713,
	0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38,
	0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21,
	0x86d3d2d4, 0xf1d4e242, 0x68ddb3f8, 0x1fda836e,
	0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
	0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c,
	0x8f659eff, 0xf862ae69, 0x616bffd3, 0x166ccf45,
	0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2,
	0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db,
	0xaed16a4a, 0xd9d65adc, 0x40df0b66, 0x37d83bf0,
	0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
	0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6,
	0xbad03605, 0xcdd70693, 0x54de5729, 0x23d967bf,
	0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94,
	0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d,
};





void chomp(char *s)
{
	while (*s && *s != '\n' && *s != '\r') s++;
	*s = 0;
}


int some_exec(char *cmd)
{
	FILE *pipein_fp;
	char readbuf[1024];
	static int retcode;
	signal(SIGCLD, SIG_DFL);
	char pipe_out[1024];

	memset(pipe_out, 0, 1024);
	clog(info, CDEBUG, DBG_SYSTEM,"SHELL: '%s'", cmd);
	if ((pipein_fp = popen(cmd, "r")) == NULL)
	{
		clog(info, CERROR, DBG_SYSTEM,"F:%s: Can't exec route command.", __FUNCTION__);
		return -1;
	}

	while (fgets(readbuf, 1024, pipein_fp))
	{
		strcat(pipe_out, readbuf);
		strcat(pipe_out, "\r");
	}
	chomp(pipe_out);


	retcode =  pclose(pipein_fp);
//      printf("OUT=%s ret=%i errno=%i\n", pipe_out, retcode, errno);
	if (retcode != 0)
	{
		clog(info, CERROR, DBG_SYSTEM, "F:%s: ERROR: '%s'", __FUNCTION__, pipe_out);
	}
	return retcode;
}



unsigned int mycrc32(u_char *buffer, unsigned length)
{
	unsigned i;
	unsigned int crc32 = ~0L;
	for (i = 0; i < length; i++)
		CRC32C(crc32, (unsigned char)buffer[i]);
	return ~crc32;
} 

void hex_dump(void *data, int size)
{
	/* dumps size bytes of *data to stdout. Looks like:
	   * [0000] 75 6E 6B 6E 6F 77 6E 20
	   *                  30 FF 00 00 00 00 39 00 unknown 0.....9.
	   * (in a single line of course)
	   */

	unsigned char *p = data;
	unsigned char c;
	int n;
	char bytestr[4] = {0};
	char addrstr[20] = {0};
	char hexstr[16 * 3 + 5] = {0};
	char charstr[16 * 1 + 5] = {0};
	for (n = 1; n <= size; n++)
	{
		if (n % 16 == 1)
		{
			/* store address for this line */
			//            snprintf(addrstr, sizeof(addrstr), "%.4x",
			//               ((unsigned int)p-(unsigned int)data) );
			snprintf(addrstr, sizeof(addrstr), "%04u", n - 1);
		}

		c = *p;
		if (isalnum(c) == 0)
		{
			c = '.';
		}

		/* store hex str (for left side) */
		snprintf(bytestr, sizeof(bytestr), "%02X ", *p);
		strncat(hexstr, bytestr, sizeof(hexstr) - strlen(hexstr) - 1);

		/* store char str (for right side) */
		snprintf(bytestr, sizeof(bytestr), "%c", c);
		strncat(charstr, bytestr, sizeof(charstr) - strlen(charstr) - 1);

		if (n % 16 == 0)
		{
			/* line completed */
			//            printf("[%10.10s]   %-50.50s  %s\n", addrstr, hexstr, charstr);
			clog(info, CMARK, DBG_SYSTEM, "[%6.6s]   %-50.50s  %s", addrstr, hexstr, charstr);
			hexstr[0] = 0;
			charstr[0] = 0;
		} else if (n % 8 == 0)
		{
			/* half line: add whitespaces */
			strncat(hexstr, "  ", sizeof(hexstr) - strlen(hexstr) - 1);
			strncat(charstr, " ", sizeof(charstr) - strlen(charstr) - 1);
		}
		p++; /* next byte */
	}

	if (strlen(hexstr) > 0)
	{
		/* print rest of buffer if not empty */
		//        printf("[%10.10s]   %-50.50s  %s\n", addrstr, hexstr, charstr);
		clog(info, CMARK, DBG_SYSTEM, "[%6.6s]   %-50.50s  %s", addrstr, hexstr, charstr);
	}
}


u_int32_t rand32()
{
	u_int32_t x;
	x = rand() & 0xff;
	x |= (rand() & 0xff) << 8;
	x |= (rand() & 0xff) << 16;
	x |= (rand() & 0xff) << 24;    
	return x;
}


char *arp_op(u_short op)
{
	switch (op)
	{
	case 1:
		return "(ARP Request)     ";
		break;
	case 2:
		return "(ARP Response)    ";
		break;
	case 3:
		return "(RARP Request)    ";
		break;
	case 4:
		return "(RARP Response)   ";
		break;
	case 5:
		return "(Dyn RARP request)";
		break;
	case 6:
		return "(Dyn RARP reply)  ";
		break;
	case 7:
		return "(Dyn RARP error)  ";
		break;
	case 8:
		return "(InARP request)   ";
		break;
	case 9:
		return "(InARP reply)     ";
		break;
	default:
		return "(?)               ";
	}
}

void unip_debug_packet(u_char *pkt, u_int32_t is_rx, u_char *tag)
{
	struct eth_hdr *ether = (struct eth_hdr *)pkt;
	char eproto[32];

	int epn = ntohs(ether->type);

	switch (epn)
	{
	case ETHTYPE_IP:
		sprintf(eproto, "IP");
		break;
	case ETHTYPE_ARP:
		sprintf(eproto, "ARP");
		break;
	case ETHTYPE_IPV6:
		sprintf(eproto, "IP6");
		break;
	case ETHTYPE_VLAN:
		sprintf(eproto, "VLAN");
	default:
		sprintf(eproto, "%02X", ntohs(ether->type));
		break;
	}

	// ARP
	if (epn == ETHTYPE_ARP)
	{
		struct etharp_hdr *arp = (struct etharp_hdr *)(pkt + sizeof(struct eth_hdr));
		clog(info, CMARK, DBG_SYSTEM, "%s: %s T:%-3s  %02X:%02X:%02X:%02X:%02X:%02X -> %02X:%02X:%02X:%02X:%02X:%02X  %-2s",  tag, is_rx ? "[RX]" : "[TX]",
			 eproto, 
			 ether->src.addr[0], ether->src.addr[1], ether->src.addr[2], ether->src.addr[3], ether->src.addr[4], ether->src.addr[5],
			 ether->dest.addr[0], ether->dest.addr[1], ether->dest.addr[2], ether->dest.addr[3], ether->dest.addr[4], ether->dest.addr[5],
			 arp_op(htons(arp->opcode)));
		return;
	}

	if (epn == ETHTYPE_IP)
	{
//		printf(">>>>>>>>>>>>>>>>>>>> RX IP <<<<<<<<<<<<<<<<<<<<<<< RX=%s\n", is_rx?"YES":"NO");
		u_char l3_addon[128];
		// Only TX
		char sproto[32];
		char srcAddr[20], dstAddr[20];
		struct sniff_ip *ip = (struct sniff_ip *)(pkt + sizeof(struct eth_hdr));

		sprintf(l3_addon, "");

		sprintf(srcAddr, inet_ntoa(ip->ip_src));
		sprintf(dstAddr, inet_ntoa(ip->ip_dst));
		switch (ip->ip_p)
		{
		case IPPROTO_TCP:
			sprintf(sproto, "TCP");
			break;
		case IPPROTO_UDP:
			sprintf(sproto, "UDP");
			break;
		case IPPROTO_ICMP:
			sprintf(sproto, "ICMP");
			break;
		default:
			sprintf(sproto, "?0x%x", ip->ip_p);        
			break;
		}

		if (ip->ip_p == IPPROTO_UDP)
		{
			struct udphdr *udph = (struct udphdr *) (pkt + sizeof (struct eth_hdr )  + sizeof (struct sniff_ip));
			sprintf(l3_addon, " %u:%u",  (unsigned) ntohs(udph->uh_sport ), (unsigned) ntohs(udph->uh_dport));
		}

		clog(info, CMARK, DBG_SYSTEM, "%s: %s T:%-3s  %02X:%02X:%02X:%02X:%02X:%02X -> %02X:%02X:%02X:%02X:%02X:%02X  %-2s %s -> %s  %s", tag, is_rx ? "[RX]" : "[TX]",
			 eproto, 
			 ether->src.addr[0], ether->src.addr[1], ether->src.addr[2], ether->src.addr[3], ether->src.addr[4], ether->src.addr[5],
			 ether->dest.addr[0], ether->dest.addr[1], ether->dest.addr[2], ether->dest.addr[3], ether->dest.addr[4], ether->dest.addr[5],
			 sproto, srcAddr, dstAddr, l3_addon);
		return;
	}

	clog(info, CMARK, DBG_SYSTEM, "%s: %s T:%-3s  %02X:%02X:%02X:%02X:%02X:%02X -> %02X:%02X:%02X:%02X:%02X:%02X", tag, is_rx ? "[RX]" : "[TX]",
		 eproto, 
		 ether->src.addr[0], ether->src.addr[1], ether->src.addr[2], ether->src.addr[3], ether->src.addr[4], ether->src.addr[5],
		 ether->dest.addr[0], ether->dest.addr[1], ether->dest.addr[2], ether->dest.addr[3], ether->dest.addr[4], ether->dest.addr[5]);
} 


int is_eth_broadcast(struct eth_addr *eaddr)
{
	u_char broadcast_mac[ETH_HWADDR_LEN];
	memset(broadcast_mac, 0xFF, ETH_HWADDR_LEN);
	if (memcmp(eaddr->addr, broadcast_mac, ETH_HWADDR_LEN) == 0)
		return 1;
	return 0;
}


void segv_handler(int sig)
{
	void *array[10];
	size_t size;

	// get void*'s for all entries on the stack
	size = backtrace(array, 10);

	// print out all the frames to stderr
	fprintf(stderr, "Error: signal %d:\n", sig);
	backtrace_symbols_fd(array, size, STDERR_FILENO);
	exit(1);
}

int iface_get_hwaddr(char *ifname, char *hwaddr)
{
	int fd;
	int ret = 0;
	struct ifreq ifr;
	unsigned char *mac;

	memset(hwaddr, 0, 6);
	fd = socket(AF_INET, SOCK_DGRAM, 0);
	ifr.ifr_addr.sa_family = AF_INET;
	strncpy(ifr.ifr_name , ifname , IFNAMSIZ-1);
	ret = ioctl(fd, SIOCGIFHWADDR, &ifr);
	close(fd);
	memcpy(hwaddr, (void *)ifr.ifr_hwaddr.sa_data, 6);
	return ret;
}



u_int16_t in_cksum(unsigned short *addr, int len)
{
	register int sum = 0;
	u_short answer = 0;
	register u_short *w = addr;
	register int nleft = len;
	/*
	 * Our algorithm is simple, using a 32 bit accumulator (sum), we add
	 * sequential 16 bit words to it, and at the end, fold back all the
	 * carry bits from the top 16 bits into the lower 16 bits.
	 */
	while (nleft > 1)
	{
		sum += *w++;
		nleft -= 2;
	}
	/* mop up an odd byte, if necessary */
	if (nleft == 1)
	{
		*(u_char *)(&answer) = *(u_char *) w;
		sum += answer;
	}
	/* add back carry outs from top 16 bits to low 16 bits */
	sum = (sum >> 16) + (sum & 0xffff);		/* add hi 16 to low 16 */
	sum += (sum >> 16);				/* add carry */
	answer = ~sum;				/* truncate to 16 bits */
	return(answer);
}




char *uuid_gen()
{
	srand (clock());
	static char GUID[40];
	int t = 0;
	char *szTemp = "xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx";
	char *szHex = "0123456789abcdef-";
	int nLen = strlen (szTemp);

	for (t=0; t<nLen+1; t++)
	{
		int r = rand () % 16;
		char c = ' ';

		switch (szTemp[t])
		{
		case 'x' : { c = szHex [r];} break;
		case 'y' : { c = szHex [r & 0x03 | 0x08];} break;
		case '-' : { c = '-';} break;
		case '4' : { c = '4';} break;
		}

		GUID[t] = ( t < nLen ) ? c : 0x00;
	}
	return GUID;
}



/* UUID handler */

char *mux_uuid()
{
	// First try to read uuid file
	char file_uuid[64];

	sprintf(file_uuid, "");

	FILE *fp = fopen(MUX_UUID_FILE, "r+");
	if (fp != NULL)
	{
		fgets(file_uuid, 64, fp);
		fclose(fp);
	}

	if (strlen(file_uuid) != MUX_UUID_LEN)
	{
		char *uuid = uuid_gen();
		clog(info, CMARK, DBG_SYSTEM, "F:%s: Unable to read '%s' file. Generage new UUID: '%s'", __FUNCTION__, MUX_UUID_FILE, uuid);
		// Write to file

		fp = fopen(MUX_UUID_FILE, "w");
		if (fp != NULL)
		{
			// file opened
			fputs(uuid, fp);
			fclose(fp);
		} else
		{
			clog(info, CWARN, DBG_SYSTEM, "F:%s: Can't write uuid file '%s' Err:'%s'", __FUNCTION__, MUX_UUID_FILE, strerror(errno));
		}
	} else
	{
		clog(info, CMARK, DBG_SYSTEM, "F:%s: My UUID: %s", __FUNCTION__, file_uuid);
	}
	return  strdup(file_uuid);
}




/* Transport util */
MuxTransport *mux_transport_init(MuxTransport *tr_type)
{
	MuxTransport *tr = (MuxTransport *) malloc(sizeof(MuxTransport));
	memset(tr, 0, sizeof(MuxTransport));
	memcpy(tr, tr_type, sizeof(MuxTransport));	
	tr->priv = 0;
	tr->priv = tr->tr_init(tr);

	if (tr->priv == NULL)
	{
		clog(info, CMARK, DBG_SYSTEM, "F:%s: Transport '%s' init >>FAILED<<", __FUNCTION__, tr_type->name);
		free(tr);
		return (MuxTransport *) NULL;
	}
	clog(info, CMARK, DBG_SYSTEM, "F:%s: Transport '%s' init done", __FUNCTION__, tr_type->name);
	return tr;

}

void mux_transport_destroy(MuxTransport *tr)
{
	if (!tr) return;
	if (tr->tr_destroy)
		tr->tr_destroy(tr->priv);
	free(tr);
}



/* Humanization :) */


/* human-readable size */
void human_size(char *buf, int size, u_int64_t bytes)
{
        int i;
        char unit[] = "bKMGT";
        for (i = 0; bytes >= 10000 && unit[i] != 'T'; i++)
                bytes = (bytes + 512) / 1024;
        snprintf(buf, size, "%lu%c%s", (unsigned long) bytes, unit[i], i ? "B" : " ");
}


/* human-readable rate */
void human_rate(char *buf, int len, u_int32_t rate)
{
        double tmp = (double) rate * 8;
        int use_iec = 0;
        if (use_iec)
        {
                if (tmp >= 1000.0*1024.0*1024.0)
                        snprintf(buf, len, "%.0fMibit", tmp/(1024.0*1024.0));
                else if (tmp >= 1000.0*1024)
                        snprintf(buf, len, "%.0fKibit", tmp/1024);
                else
                        snprintf(buf, len, "%.0fbit", tmp);
        }
        else
        {
                if (tmp >= 100.0*10000.0)
                        snprintf(buf, len, "%.1fMbit", tmp/1000000.0);
                else
                        snprintf(buf, len, "%.1fKbit", tmp/1000.0);
        }
}

