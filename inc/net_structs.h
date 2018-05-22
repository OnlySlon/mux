#pragma pack(1)

#define ETH_HWADDR_LEN    6
#define SIZEOF_ETH_HDR    14
#define SIZEOF_VLAN_HDR   4
#define SIZEOF_IP_HDR     20
#define SIZEOF_UDP_HDR    8



/* The 24-bit IANA IPv4-multicast OUI is 01-00-5e: */
#define LL_IP4_MULTICAST_ADDR_0 0x01
#define LL_IP4_MULTICAST_ADDR_1 0x00
#define LL_IP4_MULTICAST_ADDR_2 0x5e

/* IPv6 multicast uses this prefix */
#define LL_IP6_MULTICAST_ADDR_0 0x33
#define LL_IP6_MULTICAST_ADDR_1 0x33


struct eth_addr 
{
	u_int8_t addr[ETH_HWADDR_LEN];
};

struct eth_hdr {

  struct eth_addr  dest;
  struct eth_addr  src;
  u_int16_t        type;
};

/* 
 
./PF_RING/kernel/linux/pf_ring.h:171:8: note: originally defined here 
 
struct eth_vlan_hdr {
  u_int16_t prio_vid;
  u_int16_t tpid;
};
 
*/ 


enum lwip_ieee_eth_type {
  /* Internet protocol v4 */
  ETHTYPE_IP        = 0x0800U,
  /* Address resolution protocol */
  ETHTYPE_ARP       = 0x0806U, 
  /* Wake on lan */
  ETHTYPE_WOL       = 0x0842U,
  /* RARP */
  ETHTYPE_RARP      = 0x8035U,
  /* Virtual local area network */
  ETHTYPE_VLAN      = 0x8100U,
  /* Internet protocol v6 */
  ETHTYPE_IPV6      = 0x86DDU,
  /* PPP Over Ethernet Discovery Stage */
  ETHTYPE_PPPOEDISC = 0x8863U,
  /* PPP Over Ethernet Session Stage */
  ETHTYPE_PPPOE     = 0x8864U,
  /* Jumbo Frames */
  ETHTYPE_JUMBO     = 0x8870U,
  /* Process field network */
  ETHTYPE_PROFINET  = 0x8892U,
  /* Ethernet for control automation technology */
  ETHTYPE_ETHERCAT  = 0x88A4U,
  /* Link layer discovery protocol */
  ETHTYPE_LLDP      = 0x88CCU,
  /* Serial real-time communication system */
  ETHTYPE_SERCOS    = 0x88CDU,
  /* Media redundancy protocol */
  ETHTYPE_MRP       = 0x88E3U,
  /* Precision time protocol */
  ETHTYPE_PTP       = 0x88F7U,
  /* Q-in-Q, 802.1ad */
  ETHTYPE_QINQ      = 0x9100U
};





struct etharp_hdr 
{
  u_int16_t         hwtype;
  u_int16_t         proto;
  u_int8_t          hwlen;
  u_int8_t          protolen;
  u_int16_t         opcode;
  struct eth_addr   shwaddr;
  u_int32_t         sipaddr;
  struct eth_addr   dhwaddr;
  u_int32_t         dipaddr;
};

#define SIZEOF_ETHARP_HDR 28

/* ARP message types (opcodes) */
enum etharp_opcode {
  ARP_REQUEST = 1,
  ARP_REPLY   = 2
};




/* IP header */
struct sniff_ip
{
    u_char ip_vhl;                   /* version << 4 | header length >> 2 */
    u_char ip_tos;                   /* type of service */
    u_short ip_len;                  /* total length */
    u_short ip_id;                   /* identification */
    u_short ip_off;                  /* fragment offset field */
#define IP_RF 0x8000                 /* reserved fragment flag */
#define IP_DF 0x4000                 /* dont fragment flag */
#define IP_MF 0x2000                 /* more fragments flag */
#define IP_OFFMASK 0x1fff            /* mask for fragmenting bits */
    u_char ip_ttl;                   /* time to live */
    u_char ip_p;                     /* protocol */
    u_short ip_sum;                  /* checksum */
    struct in_addr ip_src, ip_dst;   /* source and dest address */
};
#define IP_HL(ip) (((ip)->ip_vhl) & 0x0f)
#define IP_V(ip) (((ip)->ip_vhl) >> 4)



struct udphdr {
	uint16_t	uh_sport;		/* source port */
	uint16_t	uh_dport;		/* destination port */
	uint16_t	uh_ulen;		/* udp length */
	uint16_t	uh_sum;			/* udp checksum */
}; 


enum lwip_iana_hwtype {
  /** Ethernet */
  IANA_HWTYPE_ETHERNET = 1
};





/* ----------------------- IP -------------------------- */

#define IP_HLEN 20
/* Maximum size of the IPv4 header with options. */
#define IP_HLEN_MAX 60



struct ip_hdr {
  u_int8_t    _v_hl;         /* version / header length */
  u_int8_t    _tos;          /* type of service */
  u_int16_t   _len;          /* total length */
  u_int16_t   _id;           /* identification */
  u_int16_t   _offset;       /* fragment offset field */
#define IP_RF 0x8000U        /* reserved fragment flag */
#define IP_DF 0x4000U        /* don't fragment flag */
#define IP_MF 0x2000U        /* more fragments flag */
#define IP_OFFMASK 0x1fffU   /* mask for fragmenting bits */
  u_int8_t    _ttl;          /* time to live */  
  u_int8_t    _proto;        /* protocol */
  u_int16_t   _chksum;       /* checksum */
  u_int32_t   src;
  u_int32_t   dst;
};


#define IP_TOS_DEFAULT 0x10
#define IP_TTL_DEFAULT 127



#define MUX_IPPAYLOAD_OFF (SIZEOF_ETH_HDR+IP_HLEN)
#pragma pack(0)

