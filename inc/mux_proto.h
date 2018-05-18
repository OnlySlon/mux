
#pragma pack(1)


#define MUX_PROTO_TPE_DATA          0x00       // simply ip packet 
#define MUX_PROTO_TPE_UNK1          0x01       // --- UNUSED ---
#define MUX_PROTO_TPE_DISCOVERY     0x02       // server discovery packet
#define MUX_PROTO_TPE_HANDSHAKE     0x03       // handshake procedure    
#define MUX_PROTO_TPE_OP            0x04       // some single op
#define MUX_PROTO_TPE_RTTEST        0x05       // RTT estimator packet
#define MUX_PROTO_TPE_UNK2          0x06       // --- UNUSED ---
#define MUX_PROTO_TPE_ALIVE         0x07       // Keep-alive packet
#define MUX_PROTO_TPE_DICTCFG       0x08       // dictionary config 

#define MUX_PROTO_F_COMPRESSED      (1 << 0)   // payload is compressed
#define MUX_PROTO_F_HAVECRC         (1 << 1)   // payload have crc
#define MUX_PROTO_F_HAVESEQINFO     (1 << 2)   // payload have SeqInfo
#define MUX_PROTO_UNKNOWN3          (1 << 3)   // --- UNUSED ---
#define MUX_PROTO_UNKNOWN4          (1 << 4)   // --- UNUSED ---



// L0 3 bytes
typedef struct MuxProto_Header
{
	u_int8_t     type:3;	// packet type (9)
	u_int8_t     flags:5;	// flags 5 bit
	u_int16_t    tunid;
} MuxProtoHdr;

typedef struct MuxProto_Crc
{
	u_int16_t   crc;
} MuxProtoCrc;


//  uses then MUX_PROTO_TPE_DATASEQ
typedef struct MuxProto_SeqInfo
{
	u_int16_t  seq_global;
	u_int16_t  seq_local;
} MuxProtoSeqInfo;



#define MUX_PROTO_SSHAKE_REQUEST 0x01
#define MUX_PROTO_SSHAKE_REPLY   0x02

typedef struct MuxProto_handshake
{
	u_int8_t    stage;
	char        GUID[40];
	u_int16_t   payload_size;
	u_int16_t   tunid;
} MuxProtoHandshakeT;


#pragma pack(0)


#define MUX_SERVER 0x00
#define MUX_CLIENT 0x01


