#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncTCPbuffer.h>

typedef unsigned char uchar;
typedef unsigned short int usint;
//typedef unsigned int uint;

typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned int uint32;
typedef unsigned long long uint64;

struct cc_crypt_block
{
	uint8 keytable[256];
	uint8 state;
	uint8 counter;
	uint8 sum;
} __attribute__((packed));



#define CC_MAXMSGSIZE	512

typedef enum
{
  CC_MSG_CLI_INFO,			// client -> server
  CC_MSG_ECM_REQUEST,		// client -> server
  CC_MSG_EMM_REQUEST,		// client -> server
  CC_MSG_CARD_DEL = 4,		// server -> client
  CC_MSG_BAD_ECM,
  CC_MSG_KEEPALIVE,		// client -> server
  CC_MSG_CARD_ADD,			// server -> client
  CC_MSG_SRV_INFO,			// server -> client
  CC_MSG_CMD_0B = 0x0b,	// server -> client ???????
  CC_MSG_ECM_NOK1 = 0xfe,	// server -> client ecm queue full, card not found
  CC_MSG_ECM_NOK2 = 0xff,	// server -> client
  CC_MSG_NO_HEADER = 0xffff
} cc_msg_cmd;


typedef enum
{
  CC_STAT_CONNETED,			// client -> server
  CC_STAT_LOGIN,		// client -> server
  CC_STAT_RUN,		// client -> server
  // CC_STAT_SEND_USER,		// server -> client
  // CC_STAT_SEND_PASS,
  // CC_STAT_SEND_INFO,		// client -> server
  // CC_STAT_,			// server -> client
  // CC_STAT_,			// server -> client
  // CC_STAT_ ,	// server -> client ???????
  // CC_STAT_ ,	// server -> client ecm queue full, card not found
  // CC_STAT_ ,	// server -> client
  // CC_STAT_
} cc_stat;




class cccam
{
private:
    // WiFiClient client;
    struct cc_crypt_block *recvblock;
    struct cc_crypt_block *sendblock;
	void cc_rc4_crypt(struct cc_crypt_block *block, uint8 *data, int len,uint8 mode);
	uint8 nodeidd[8];
	uint32 busycardid;
	char user[64];
    char pass[64];
    unsigned char nodeid[8];
    int busy; // cardserver is busy (ecm was sent) / or not (no ecm sent/dcw returned)
	int keepalivesent;
	int is_linked;
	WiFiClient client;
	unsigned char g_buf[512];
	unsigned char *send_buf;
	char CC_server[32];
    uint16_t CC_port;  
	AsyncClient *tcpclient;
	AsyncTCPbuffer *tcpbuffer	;
	void handle_hello() ;
	int data_call_back();
	cc_stat zuangtai;
	bool cc_msg_recv_stat;
	int data_len;
	


public:
    int cc_msg_send( cc_msg_cmd cmd, int len, uint8 *buf);
    int cc_msg_recv_nohead(  uint8 *buf, int len);
    int cc_msg_chkrecv();
    int cc_msg_recv( uint8 *buf);
    void cc_crypt_cw(uint8 *nodeid/*client node id*/, uint32 card_id, uint8 *cws);
    void cc_encrypt(struct cc_crypt_block *block, uint8 *data, int len);
    void cc_decrypt(struct cc_crypt_block *block, uint8 *data, int len);
    void cc_crypt_xor(uint8 *buf);
    void cc_crypt_init( struct cc_crypt_block *block, uint8 *key, int len);
    void cc_crypt_swap(unsigned char *p1, unsigned char *p2);
    int cc_getcardbyid(  uint32 id );
    int cc_sendinfo_srv();
    int cc_connect_srv( );
    void cc_disconnect_srv();
    int cc_srv_recvmsg(uint8 *buf);
    int send_ecm(uint8 *buf,int len);
    int recv_nonb(uint8 *buf, int len);
    int send_nonb( unsigned char *, int);
    void setup();
	boolean net_init();
	int  get_cw(uint8 *buffer);
	void hex_dump(const unsigned char *buf, size_t len);
	int    Keepalive(void);
	void Setup(char *CCCam_server, uint16_t CCCam_port,char *CCCam_user,char *CCCam_pass);
	void runAsyncClient();
};
