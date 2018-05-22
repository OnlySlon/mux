
#include "uthash.h"
#include "clog.h"
#include "pfring.h"
#include "timers.h"
#include "net_structs.h"


#define MUX_UUID_FILE "/etc/mux_uuid"
#define MUX_UUID_LEN  36

#define MUX_IFNAME_MAXLEN 40
#define  MUX_PROTONUM 5

#include "mux_proto.h"


#define MUX_PACKET_PASS 0x00
#define MUX_PACKET_DROP 0x01

enum MuxTransportT {
    MUX_TR_RAW = 0,
    MUX_TR_UDP
};

typedef struct MUX_TRANSPORT_ENDPOINT_S
{
	u_int32_t type;
	u_int32_t host4;
	u_int16_t port;
} MuxTrandportEP;

typedef struct MUX_NETLIST4
{
	int              id;
	int              mask;
	UT_hash_handle   hh;
} MuxNetlist4;


#define CLIENT_ID_LEN 32
typedef struct MUX_CLIENT_S
{
	int              id;
	char             id_int[CLIENT_ID_LEN];
	MuxNetlist4     *netlist;
	UT_hash_handle   hh;
} MuxClient;


/* uses for store nets & associate with clients */
typedef struct MUX_NETLIST4_ASSOC
{
	int              id;
	MuxClient       *client;
	MuxNetlist4     *net;
	UT_hash_handle   hh;
} MuxNetAssoc;



enum MuxIfTpe
{
	MUX_IF_BRIDGE = 0,
	MUX_IF_TUN,
	MUX_IF_TAP,
	MUX_IF_RAW,
	MUX_IF_RING
};


typedef struct MUXCTX_S
{
	CLOG_INFO        *info;
	struct pollfd    *polldb;
	int               fds_allocated;
} MUXCTX;



#define MUX_IPCFG_F_SECONDARY (1 << 0)

typedef struct MUXIPCFG_S
{
	// ipv4 only
	u_int32_t          ipaddr;
	u_int32_t          mask;
	u_int32_t          gateway;
	u_int32_t          flags;
	UT_hash_handle     hh;
} MuxIPCfg;





#define MUX_IF_NAME_LEN 32
#define MUX_IF_F_SLAVE        0 
#define MUX_IF_F_MASTER       (1 << 0)
#define MUX_IF_F_DISABLED     (1 << 1)
#define MUX_IF_F_GWDISCOVERY  (1 << 2)
#define MUX_IF_F_CLIENT       (1 << 3)
#define MUX_IF_F_SRVDISCOVERY (1 << 4)


typedef struct MUX_IF_STATS
{
	u_int64_t    tx_bytes;
	u_int64_t    tx_packets;
	u_int64_t    tx_errors;
	u_int64_t    tx_rate;
	

	u_int64_t    rx_bytes;
	u_int64_t    rx_packets;
	u_int64_t    rx_errors;
	u_int64_t    rx_rate;
} MuxIfStats;

typedef struct MUX_IFS_S
{
	int                id;
	u_int32_t          type;											// Internal interface type
	char               name[MUX_IF_NAME_LEN];							// Internal interface name
	char               hwname[MUX_IF_NAME_LEN];							// In-system interface name
	pfring            *ring;											// pointer to PF_RING struct
	int                ring_ifindex;									// PF_RING if index
	int                fd;												// TUN & TAP file descriptor
	int                (*rx_callback) (MUXCTX *ctx, void *rif);	// RX callback function
	int                (*tx_callback) (MUXCTX *ctx, void *rif, char *pkt, u_int32_t len);		 // TX function
	int                br_id;											// Bridge ID
	int                is_bridge;										// This interface in bridge?
	u_int32_t          flags;
	struct eth_addr    hwaddr;
	MuxIPCfg          *ip;
	MuxIfStats         stats;
	UT_hash_handle     hh;												// UThash service handler
} MuxIf_t;








enum MuxIfConfT {
    MUX_IFCONF_MANUAL = 0,   // Manual setup ip, mask and gw
    MUX_IFCONF_DHCP,         // DHCP discovery
    MUX_IFCONF_SNIFF,        // Hardcore discovery 
    MUX_IFCONF_CDP           // CDP reincatnation for Specific hardware
};

enum MufIfGetT
{
    MUX_IFCONF_GETSTATUS,
    MUX_IFCONF_GETADDR,
    MUX_IFCONF_GETMASK,
    MUX_IFCONF_GETGW,
};

typedef struct MUX_IFCONF_PLUGIN_S
{
    u_int32_t  type;             // plugin type
    char      *name;         // plugin name
    char       bpf_filter[64];   // bpf filter for incoming traffic. Pass to plugin
    int        (*init_cb)  (void *priv);
    int        (*rx_cb)    (void *priv, char *pkt, u_int32_t len);
    int        (*get_info4)(void *priv, void *data, u_int32_t *len);

    void       *priv;            // private plugin data
} MuxIfConf_t;



/* transport plugin */


#define MUX_TR_OP_SETIF          0x00   // link transport with interface
#define MUX_TR_OP_SETGWHW        0x01   // setup gateway hardware address for transport
#define MUX_TR_OP_SETREMOTE      0x02   // setup remote endpoint address
#define MUX_TR_OP_HZ             0xFF
typedef struct MUX_TR_PLUGIN
{
	u_int32_t   type;
	char       *name;
	int         (*tr_init)(MuxIf_t *tif);
	int         (*tr_send)(void *priv, char *pkt, int len);
	int         (*tr_sendto)(void *priv, u_int32_t ipaddr,  char *pkt, int len);
	int         (*tr_recv)(void *priv, char *pkt);
	int         (*tr_ctl)(void *priv, u_int32_t op, void *op_val, int oplen);
	int         (*tr_destroy)(void *priv);
	u_int32_t    payload_offset;
	void        *priv;
} MuxTransport;


struct fdb_record
{
	int               id;
	struct eth_addr   eaddr;
	u_int32_t         ttl;
	u_int32_t         br_id;  // bridge id
	MuxIf_t          *iface;
	UT_hash_handle    hh;    
};


typedef struct arp_record 
{
    int               id; // key = ip address
    struct eth_addr   arp;
    int               state;
    struct in_addr    ip;
	int               ctime;
    MuxIf_t          *ifr;
	u_int32_t         hits; // resolve hits
    UT_hash_handle    hh;
} ArpRecord ;



/*
 
 VM1       VM2
 eth0 ---- eth1
 
*/

/* >>>>>>>>>>>>>>>>>> GATEWAY DISCOVERY  <<<<<<<<<<<<<<<<<<<<<<<<< */

enum MuxDiscoveryState {
    MUX_GWDISCOVERY_PRETENDER = 0,
    MUX_GWDISCOVERY_COMMITED,
	MUX_GWDISCOVERY_RECHECK,
};

#define MUX_GWDISCOVERY_TTL            60          // 60 sec.
#define MUX_GWDISCOVERY_TTL_RECHECK    10 

typedef struct mux_gwdiscovery_info
{
	int                id;     // key in this case - 48to32 bit hash of gateway mac address
	u_int32_t          state;
	struct eth_addr    gw_hw;
	struct eth_addr    cli_hw;
	struct in_addr     cli_ip;
	struct in_addr     gw_ip;  // We don't know gateway ip address on gwdiscovery mode
	u_int32_t          ttl;
	MuxIf_t          *iface;
	MuxIf_t          *iface_gw;
	UT_hash_handle     hh;
} MuxGwDiscovery;


/* ------------------------ MUX tuns structures -------------------------------  */

//#define MUX_UI_SIZE   40

#define MUX_INSTANCE_STAGE_SRV_GOTSSHAKE  0x01
#define MUX_INSTANCE_STAGE_SRV_GOTTUNS    0x02


#define MUX_DEVSTAGE_INITIAL 0x00
#define MUX_DEVSTATE_ATWORK  0x01

typedef struct mux_devinstance
{
	int                id;
	char               UUID[MUX_UUID_LEN];
	u_int32_t          stage;
	u_int32_t          group_id;
	UT_hash_handle     hh; 
} MuxDevInstance;



// Multi-Tun side of tunnel
typedef struct mux_muxtun
{
	int                id;
	u_int16_t          id_global;
	u_int32_t          ip_remote;
	u_int32_t          ip_local;
	u_int32_t          group;
	u_int32_t          state;
	MuxTransport      *tr;
	u_int32_t          seq;
	MuxDevInstance    *device;
	char               ifname_remote[MUX_IFNAME_MAXLEN];
	UT_hash_handle     hh; 
} MuxMuxTun;



enum MuxMuxStates
{
	MUX_MUXSTATE_DOWN = 0,
	MUX_MUXSTATE_UP,
};



#define MUX_TR_F_DSTHW_EXTST (1 << 0)
