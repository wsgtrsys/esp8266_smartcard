#if defined(ARDUINO) && ARDUINO >= 100
#include <Arduino.h>
#else
#include "WProgram.h"
#endif

#include <Hash.h>
#include "cccam.h"


// #define DBGmsg Serial.printf
#define DBGmsg(c,...) do { ; } while(0)
	

extern uint8_t ecm_cw[17];

void cccam::runAsyncClient()
{

    	
	tcpclient = new AsyncClient();

    if(!tcpclient)
    {
        DBGmsg("Creating AsyncClient class failed!\r\n");
        return;
    }

    tcpclient->onDisconnect([](void *obj, AsyncClient * c)
    {
        cccam *cc = (cccam *)obj;
		// if (cc->tcpclient)
		// {
			// c->free();
			// delete c;
			// cc->tcpclient = NULL;
		// }
		DBGmsg("Server Disconnect\r\n");
    }, this);

    tcpclient->onConnect(std::bind([](cccam * cc , AsyncClient * tcp)
    {
        DBGmsg("Server Connected\r\n");

    }, this, std::placeholders::_2));

    tcpclient->onData([](void *arg, AsyncClient * c, void *data, size_t len)
    {
        cccam *cc = (cccam *)arg;
        memcpy(cc->g_buf, data, len); //把随机数种复制过去

        cc->tcpbuffer = new AsyncTCPbuffer(c);
        if(! cc->tcpbuffer)
        {
            DBGmsg("creating Network class failed!\r\n");
            return;
        }
        DBGmsg("Connected\r\n");
		cc->zuangtai = CC_STAT_CONNETED;
        cc->data_call_back();

    }, this);
	
	
	    tcpclient->onTimeout([](void *arg, AsyncClient *c , uint32_t time)
    {
        cccam *cc = (cccam *)arg;

        DBGmsg("Time Out\r\n");
    }, this);



    tcpclient->onError(std::bind([](cccam * cc , AsyncClient * tcp)
    {
        DBGmsg("CCcam server onError\r\n");
		
		
		// if (cc->tcpbuffer)
    // {
        // delete cc->tcpbuffer;
        // cc->tcpbuffer = NULL;
    // }	
	
	// if (cc->tcpclient)
    // {
        // cc->tcpclient->free();
		// delete cc->tcpclient;
        // cc->tcpclient = NULL;
    // }
	
        // cc->runAsyncClient();// 重连
        cc->cc_disconnect_srv();// 重连
    }, this, std::placeholders::_2));



    if(!tcpclient->connect(CC_server, CC_port))
    {
        DBGmsg("Connecte Error\r\n");
        delete tcpclient;
		tcpclient = NULL;
    }
}


void cccam::hex_dump(const unsigned char *buf, size_t len)
{
    size_t i;

    for (i = 0; i < len; i++)
    {
        if (i && !(i % 16))
            DBGmsg("\r\n");
        DBGmsg("%02x ", *(buf + i));
    }
    DBGmsg("\r\n");
}

void cccam::Setup(char *CCCam_server, uint16_t CCCam_port, char *CCCam_user, char *CCCam_pass)
{
    strcpy(CC_server, CCCam_server);
    CC_port = CCCam_port;
    strcpy(user, CCCam_user);
    strcpy(pass, CCCam_pass);
	tcpclient = NULL;
	tcpbuffer = NULL;
	setup();
}


int  cccam::data_call_back()
{

    int n;
    uint8 *data = (uint8 *)g_buf;
    uint8 *hash = (uint8 *)g_buf + 32;
    char *pwd = (char *)g_buf + 64;
    uint8 *buf = (uint8 *)g_buf + 128; //公共 bu
    uchar a;
    uchar b;
    uchar c;
    int ismultics = 0;


    // Serial.println("on data  \n");
    // memset(data,0,128);
    // http://blog.csdn.net/han1558249222/article/details/50411442
    // cc->tcpbuffer->readBytes(data,16,std::bind(&cc::data_call_back, this, client, &(client->cHttpLine)));

    switch (zuangtai)
    {
    case CC_STAT_CONNETED:
	
		tcpbuffer->onDisconnect(std::bind([](cccam * cc, AsyncTCPbuffer * obj) -> bool {
        DBGmsg("cccam onDisconnect\r\n");
        cc->tcpclient = NULL;
		cc->tcpbuffer = NULL;
        cc->cc_disconnect_srv();
        return true;
    }, this, std::placeholders::_1));
	
	
	
        // Check Multics
        // a = (data[0]^'M') + data[1] + data[2];
        // b = data[4] + (data[5]^'C') + data[6];
        // c = data[8] + data[9] + (data[10]^'S');
        // if ( (a == data[3]) && (b == data[7]) && (c == data[11]) ) ismultics = 1;
        hex_dump(data, 16);

        cc_crypt_xor(data);  // XOR init bytes with 'CCcam'

        sha1((uint8_t *)data, 16, hash);

        cc_crypt_init(recvblock, hash, 20);
        cc_decrypt(recvblock, data, 16);
        cc_crypt_init(sendblock, data, 16);
        cc_decrypt(sendblock, hash, 20);

        cc_msg_send( CC_MSG_NO_HEADER, 20, hash);  // send crypted hash to server
        memset(buf, 0, 64);//sizeof(buf)
        memcpy(buf, user, 20);

        cc_msg_send( CC_MSG_NO_HEADER, 20, buf);    // send usr '0' padded -> 20 bytes

        memset(buf, 0, 64);//sizeof(buf)
        memset(pwd, 0, 64);//sizeof(pwd)

        memcpy(buf, "CCcam", 5);
        strncpy(pwd, pass, 63);
        cc_encrypt(sendblock, (uint8 *)pwd, strlen(pwd));
        cc_msg_send( CC_MSG_NO_HEADER, 6, buf); // send 'CCcam' xor w/ pwd
        tcpbuffer->readBytes(g_buf, 20, std::bind(&cccam::data_call_back, this));
        zuangtai = CC_STAT_LOGIN;
        break;
    case CC_STAT_LOGIN:

		cc_decrypt(recvblock, data, 20);
        if (memcmp(data, "CCcam", 5))    // check server response
        {
            DBGmsg("CCcam: login failed to Server \r\n");
            hex_dump(data, 16);
            hex_dump(buf, 16);
            return -2;
        }
        else
            DBGmsg("CCcam: login succeeded to Server  \r\n");

        if (!cc_sendinfo_srv())
        {
            DBGmsg("CCcam: server info failed to Server \r\n");
            return -3;
        }


        is_linked = 2;

        tcpbuffer->readBytes(g_buf, 4, std::bind(&cccam::data_call_back, this));
        zuangtai = CC_STAT_RUN;
        cc_msg_recv_stat = 0;//这个应该在最前面初始化。
        break;
    case CC_STAT_RUN:
        // Serial.println("cas run\n");
        cc_msg_recv(g_buf);
        break;
    }

    return 0;
}



int cccam::cc_msg_recv(uint8 *buf)
{
    int len;
    uint8 *netbuf = buf;


    if (cc_msg_recv_stat == 0)
    {
        cc_decrypt(recvblock, netbuf, 4);

        data_len = (netbuf[2] << 8) | netbuf[3];

        if (data_len != 0)    // check if any data is expected in msg
        {
            if (data_len > CC_MAXMSGSIZE - 2)
            {
                DBGmsg(" CCcam: message too big %d\r\n", ((netbuf[2] << 8) | netbuf[3]));
                // hex_dump(netbuf, 4);
				cc_disconnect_srv();
                return -1;
            }

            // DBGmsg( "CCcam: Reveive Data 0 %d \r\n", (netbuf[2] << 8) | netbuf[3]);
            tcpbuffer->readBytes(netbuf + 4, data_len, std::bind(&cccam::data_call_back, this));
            // DBGmsg( "CCcam: Reveive Data 1\r\n");
            cc_msg_recv_stat = 1;
        }
        else
        {
			cc_srv_recvmsg(ecm_cw); 
            tcpbuffer->readBytes(netbuf, 4, std::bind(&cccam::data_call_back, this));
            cc_msg_recv_stat = 0;
			// DBGmsg( "CCcam: Reveive Data 1\r\n");
        }
        // DBGmsg( "CCcam: Reveive Data 1a\r\n");
    }
    else
    {

        cc_decrypt(recvblock, netbuf + 4, data_len);
        len += 4;
        cc_msg_recv_stat = 0;
        cc_srv_recvmsg(ecm_cw); //接收数据后的处理,netbuf 里保存的是解密后的 cw，按情况应该保存为全局变量
        tcpbuffer->readBytes(netbuf, 4, std::bind(&cccam::data_call_back, this));
        // DBGmsg( "CCcam: Reveive Data 2\r\n");
    }

    // DBGmsg( "CCcam: Reveive Data 00\r\n");

    // DBGmsg( "CCcam: Reveive Data 2\r\n");
    // hex_dump(netbuf, len);

    // memcpy(buf, netbuf, len);
    return len;
}



int  cccam::cc_srv_recvmsg(uint8 *buffer)
{
    unsigned char *buf = g_buf;//等于公共buf
    int i, len, ret = 0;


    // len = cc_msg_recv( buf);
    // if (len <= 0)
    // {
    // DBGmsg(" CCcam: server read failed 0\r\n" );
    // ret = -1;//出错直接返回；
    // }
    // else if (len > 0)
    // {

    DBGmsg( "CCcam: cc_srv_recvmsg\r\n");
    // memset(buf+len,0xff,12);
    switch (buf[1])
    {
    case CC_MSG_CLI_INFO:
        DBGmsg("CCcam: Client data ACK from Server \r\n");
        break;

    case CC_MSG_ECM_REQUEST: // Get CW
        // if (!busy)
        // {
        // DBGmsg(" [!] dcw error from server , unknown ecm request");
        // break;
        // }

        DBGmsg("dcw from server\r\n");
        // hex_dump(buf,len);
        // buf[4]=0x33;

        // uint8 dcw[16];
        // memcpy(dcw ,buf+4,16);
        // hex_dump(nodeidd,8);

        cc_crypt_cw( nodeidd, busycardid, buf + 4);
        memcpy(buffer, buf + 4, 16);
		buffer[16] = 0x01;

        // hex_dump(buffer, 16);


        cc_decrypt(recvblock, buf + 4  , 16); // additional crypto step
        // busy = 0;
        ret = 1;
        break;

    case CC_MSG_ECM_NOK1: // EAGAIN, Retry

        break;

    case CC_MSG_ECM_NOK2: // ecm decode failed
        // if (!busy)
        // {
        // DBGmsg(" [!] dcw error from server (%s:%d), unknown ecm request\r\n");
        // break;
        // }
        break;

    case CC_MSG_BAD_ECM: // Add Card
        cc_msg_send(  CC_MSG_BAD_ECM, 0, NULL);
        DBGmsg("CCcam: cmd 0x05 from Server (%s:%d)\r\n");
        break;

    case CC_MSG_KEEPALIVE:
        keepalivesent = 0;
        cc_msg_send(  CC_MSG_CLI_INFO, 0, NULL);
        DBGmsg("CCcam: Keepalive ACK from Server\r\n");
        break;

    case CC_MSG_CARD_DEL: // Delete Card
        // card = card;
        break;

    case CC_MSG_CARD_ADD:
        // remove own cards -> same nodeid "cfg.cccam.nodeid"
        // if ( (buf[14]<uphops) && (buf[24]<=16) && memcmp(buf+26+buf[24]*7,cfg.cccam.nodeid,8) ) { // check Only the first 4 bytes
        // // // nodeid index = 26 + 7 * buf[24]

        // card->shareid = buf[4] << 24 | buf[5] << 16 | buf[6] << 8 | buf[7]& 0xff;
        // // card->uphops = buf[14]+1;
        // memcpy( card->nodeid, buf + 26 + buf[24] * 7, 8);
        // card->caid = (buf[12] << 8) + (buf[13]);
        // card->nbprov = buf[24];
        // card->sids = NULL;

        // i = 26+buf[24]*7;
        // DBGmsg(" CCcam: new card (%s:%d) %02x%02x%02x%02x%02x%02x%02x%02x_%x uphops %d caid %04x providers %d\r\n",host->name,port, buf[i],buf[i+1],buf[i+2],buf[i+3],buf[i+4],buf[i+5],buf[i+6],buf[i+7],card->shareid ,card->uphops, card->caid, card->nbprov);

        // DBGmsg("card add cmd, %x %x\r\n", card->caid,card->shareid);
        // hex_dump(buf,len);

        // if (card->nbprov>CARD_MAXPROV) card->nbprov = CARD_MAXPROV;
        // for (i=0;i<card->nbprov; i++) {
        // card->prov[i] = (buf[25+i*7]<<16) | (buf[26+i*7]<<8) | (buf[27+i*7]);
        // //debugf("   Provider %d = %06x\r\n",i, card->prov[i]);
        // }
        // card->next = card;
        // card = card;
        // }
        break;

    case CC_MSG_SRV_INFO:
        memcpy(nodeid, buf + 4, 8);
        // memcpy(version, buf + 12, 31);
        // for (i = 12; i < 53; i++)
        // {
        // if (!buf[i]) break;
        // if ( (buf[i] < 32) || (buf[i] > 'z') )
        // {
        // memset(version, 0, 31);
        // break;
        // }
        // }
        // memcpy(build, buf + 44, 31);

        // hex_dump(buf,len);
        // DBGmsg(" CCcam: server nodeid \r\n");
        // hex_dump(nodeid,8);
        // DBGmsg(" CCcam: server info: version %s build %s\r\n",   buf + 12, buf + 44);
        break;

    }//http://browser.oscam.cc/comp.php?repname=oscam-mirror&compare[]=/trunk/module-cccam.c@2420&compare[]=/trunk/module-cccam.c@2421
    // }
    return ret;
}

int cccam::send_nonb( unsigned char *buffer, int len )
{
    uint16_t ret;

	// DBGmsg(" CCcam: send_nonb %p %p %p\r\n",   tcpbuffer, tcpclient );//tcpclient->connected()
	if (tcpbuffer)
    ret = tcpbuffer->write(buffer, len); //直接写就可以了
	if (ret =! len ) ;//cc_disconnect_srv();
}



void cccam::setup()
{

    uint8 aaa[] = {0x79, 0x76, 0x43, 0x19, 0x8f, 0x66, 0x9a, 0x2e};
    recvblock = (struct cc_crypt_block *)malloc(sizeof(struct cc_crypt_block ));
    sendblock = (struct cc_crypt_block *)malloc(sizeof(struct cc_crypt_block ));

    memset(recvblock, 0, sizeof(struct cc_crypt_block ));
    memset(sendblock, 0, sizeof(struct cc_crypt_block ));
    memcpy(nodeidd, aaa, 8);
    busycardid = 0x8ab5823f;
    is_linked = 0;
}



void cccam::cc_crypt_swap(unsigned char *p1, unsigned char *p2)
{
    unsigned char tmp = *p1;
    *p1 = *p2;
    *p2 = tmp;
}

void cccam::cc_crypt_init( struct cc_crypt_block *block, uint8 *key, int len)
{
    int i = 0 ;
    uint8 j = 0;

    for (i = 0; i < 256; i++)
    {
        block->keytable[i] = i;
    }

    for (i = 0; i < 256; i++)
    {
        j += key[i % len] + block->keytable[i];
        cc_crypt_swap(&block->keytable[i], &block->keytable[j]);
    }

    block->state = *key;
    block->counter = 0;
    block->sum = 0;
}

///////////////////////////////////////////////////////////////////////////////
// XOR init bytes with 'CCcam'
void cccam::cc_crypt_xor(uint8 *buf)
{
    const char cccam[] = "CCcam";
    uint8 i;

    for ( i = 0; i < 8; i++ )
    {
        buf[8 + i] = i * buf[i];
        if ( i <= 5 )
        {
            buf[i] ^= cccam[i];
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
void cccam::cc_decrypt(struct cc_crypt_block *block, uint8 *data, int len)
{
    int i;
    uint8 z;

    for (i = 0; i < len; i++)
    {
        block->counter++;
        block->sum += block->keytable[block->counter];
        cc_crypt_swap(&block->keytable[block->counter], &block->keytable[block->sum]);
        z = data[i];
        data[i] = z ^ block->keytable[(block->keytable[block->counter] + block->keytable[block->sum]) & 0xff] ^ block->state;
        z = data[i];
        block->state = block->state ^ z;
    }
}



///////////////////////////////////////////////////////////////////////////////
void cccam::cc_encrypt(struct cc_crypt_block *block, uint8 *data, int len)
{
    int i;
    uint8 z;
    for (i = 0; i < len; i++)
    {
        block->counter++;
        block->sum += block->keytable[block->counter];
        cc_crypt_swap(&block->keytable[block->counter], &block->keytable[block->sum]);
        z = data[i];
        data[i] = z ^ block->keytable[(block->keytable[block->counter] + block->keytable[block->sum]) & 0xff] ^ block->state;
        block->state = block->state ^ z;
    }
}

void cccam::cc_rc4_crypt(struct cc_crypt_block *block, uint8 *data, int len,
                         uint8 mode)
{
    int i;
    uint8 z;

    for (i = 0; i < len; i++)
    {
        block->counter++;
        block->sum += block->keytable[block->counter];
        cc_crypt_swap(&block->keytable[block->counter], &block->keytable[block->sum]);
        z = data[i];
        data[i] = z ^ block->keytable[(block->keytable[block->counter]
                                       + block->keytable[block->sum]) & 0xff];
        if (!mode)
            z = data[i];
        block->state = block->state ^ z;
    }
}


///////////////////////////////////////////////////////////////////////////////
// node_id : client nodeid, the sender of the ECM Request(big endian)
// card_id : local card_id for the server
void cccam::cc_crypt_cw(uint8 *nodeid, uint32 card_id, uint8 *cws)
{
    uint8 tmp;
    int i;
    int n;
    uint8 nod[8];
    uint64 node_id;

    // uint8 aaa[]={0xff,0x76,0x43,0x19,0x8f,0x66,0x9a,0x2e};
    for (i = 0; i < 8; i++) nod[i] = nodeid[7 - i];
    for (i = 0; i < 16; i++)
    {
        if (i & 1)
            if (i != 15)
            {
                n = (nod[i >> 1] >> 4) | (nod[(i >> 1) + 1] << 4);
                // DBGmsg("n=%x\r\n",n & 0xff);
            }
            else
            {
                n = (nod[i >> 1] >> 4);
                // DBGmsg("n15=%x\r\n",n & 0xff);
            }
        else
        {
            // DBGmsg("i=%d\r\n",i);
            n = nod[i >> 1];
        }
        // if (i==15) n=0x01;//加的
        n = n & 0xff;
        tmp = cws[i] ^ n;
        if (i & 1) tmp = ~tmp;
        cws[i] = (card_id >> (2 * i)) ^ tmp;
        // DBGmsg("(%d) n=%02x, tmp=%02x, cw=%02x\r\n",i,n,tmp,cws[i]);
    }


}



int cccam::cc_msg_recv_nohead(  uint8 *buf, int len)
{
    len = recv_nonb( buf, len);  // read rest of msg
    cc_decrypt(recvblock, buf, len);
    return len;
}

///////////////////////////////////////////////////////////////////////////////
int cccam::cc_msg_send( cc_msg_cmd cmd, int len, uint8 *buf)
{
    uint8 *netbuf = g_buf;

    if (cmd == CC_MSG_NO_HEADER)
    {
        cc_encrypt(sendblock, buf, len);
        return send_nonb( buf, len);
    }
    else
    {
        netbuf[0] = 0;   // flags??
        netbuf[1] = cmd & 0xff;
        netbuf[2] = len >> 8;
        netbuf[3] = len & 0xff;
        len += 4;
        cc_encrypt(sendblock, netbuf, len);
		// DBGmsg(" CCcam: cc_msg_send %x %d\r\n",   netbuf, len );
        return send_nonb( netbuf, len);
    }
}



int cccam::cc_sendinfo_srv()
{
    uint8 *buf = send_buf;
    memset(buf, 0, CC_MAXMSGSIZE);

    memcpy(buf, user, 20);
    memcpy(buf + 20, nodeidd, 8 );
    buf[28] = 0; //wantemus;
    strcpy((char *)buf + 29, "2.0.11");	// cccam version (ascii)
    strcpy((char *)buf + 61, "2892");	// build number (ascii)
    return cc_msg_send(  CC_MSG_CLI_INFO, 20 + 8 + 1 + 64, buf);
}



void cccam::cc_disconnect_srv()
{

    // if (recvblock) free(recvblock);
    // if (sendblock) free(sendblock);
    // recvblock = NULL;
    // sendblock = NULL;
	
	
	DBGmsg("CCcam: reset canshu\r\n");

    if (recvblock) memset(recvblock,0,sizeof(struct cc_crypt_block ));
    if (sendblock) memset(sendblock,0,sizeof(struct cc_crypt_block ));

    // if (tcpbuffer)
    // {
        // delete tcpbuffer;
        // tcpbuffer = NULL;
    // }
	
	
	// if (tcpclient)
    // {
        // tcpclient->free();
		// delete tcpclient;
        // tcpclient = NULL;
    // }
	
    is_linked = 0;
}

///////////////////////////////////////////////////////////////////////////////

int  cccam::get_cw(uint8 *buffer)
{
    int i = 0;
    int ret;

    do
    {
        ret = cc_srv_recvmsg(buffer);
        i++;
    }
    while ( ret == 0 && i < 10 );

}

int    cccam::Keepalive()
{

    int ret = 0;

    // if (client.connected() && client.available())
    // {
    // ret = cc_srv_recvmsg(buffer);
    // }

    // if (client.connected())
		if ( is_linked == 0 )
    {
        send_buf = g_buf + 4;//预先留4字节
		cc_disconnect_srv();
		if (!WiFi.isConnected())
		{
			WiFi.reconnect();
		}
        runAsyncClient();
        is_linked = 1;
    }
	
	if ( is_linked == 2 )
    cc_msg_send(  CC_MSG_KEEPALIVE, 0, NULL);

    return ret;
}



int cccam::send_ecm( uint8 *ecm, int len)
{
    // unsigned char buffer_ecm_head[] =
    // {
        // 0x81, 0x70, 0x61, 0x31, 0x91, 0x08, 0xFC, 0x00, 0x0E, 0x16, 0x1E, 0x00, 0x12, 0x2F, 0x75, 0x26,
        // 0xBC, 0x63, 0x91, 0x0F, 0x04, 0x49, 0x04, 0x7F, 0x6D, 0x22, 0xA5, 0x4A, 0x91, 0x2F, 0x44, 0x12,
        // 0x43
    // }; 

	// unsigned char buffer_ecm_head[] =
    // {
        // 0x80,0x70,0x61,0x31,0x91,0x08,0xFC,0x00,0x0E,0x16,0x1E,0x00,0x12,0x2F,0x75,0x26,
		// 0xBC,0x63,0x91,0x0F,0x04,0x49,0x04,0x7F,0x6D,0x22,0xA5,0x4A,0x91,0x2F,0x44,0x12,
		// 0x43,
    // };
	unsigned char buffer_ecm_head[] =
    {
        0x80,0x70,0x61,0x84,0x76,0x08,0xFC,0x00,0x0B,0x18,0x2D,0x00,0x12,0x4E,0xA8,0x5D,
	0x88,0x7F,0xB2,0x40,0x02,0x11,0x16,0x14,0x81,0x60,0x3F,0x0D,0x39,0x5F,0x20,0x12,
	0x43,
    };

    // uint8 eccm[] =
    // {
        // 0x80, 0x70, 0x61, 0x31, 0x91, 0x08, 0xFC, 0x00, 0x0E, 0x16, 0x1E, 0x00, 0x12, 0x2F, 0x75, 0x26,
        // 0xBC, 0x63, 0x91, 0x0F, 0x04, 0x49, 0x04, 0x7F, 0x6D, 0x22, 0xA5, 0x4A, 0x91, 0x2F, 0x44, 0x12,
        // 0x43, 0x80, 0x3A, 0x08, 0xFC, 0x3E, 0x31, 0x91, 0xC0, 0x00, 0x16, 0x1F, 0x70, 0xEF, 0xE8, 0x8E,
        // 0xB3, 0xDD, 0x13, 0xB6, 0x7F, 0x2C, 0x19, 0x7B, 0x44, 0xC5, 0x94, 0xC8, 0xA0, 0xDC, 0x56, 0x48,
        // 0x0B, 0x9A, 0xAB, 0x4C, 0x52, 0x4F, 0x01, 0x09, 0x00, 0x01, 0x00, 0x01, 0x88, 0xBF, 0x74, 0xCF,
        // 0x9E, 0x74, 0x95, 0x86, 0x4C, 0x40, 0x57, 0x21, 0x2B, 0xFC, 0x1C, 0xA5, 0xD5, 0xEB, 0x1A, 0xA1,
        // 0x5D, 0x37, 0x68, 0x50,
    // };


    send_buf = g_buf + 4;//预先留4字节

    unsigned char *buf = send_buf;

    //if ( (handle>0)&&(!busy) ) {
    //	if (!cc_getcardbyid(srv, busycardid)) return 0;
    //debugf(" -> ecm to CCcam server (%s:%d) ch %04x:%06x:%04x shareid %x\r\n", host->name, port,ecm->caid,ecm->provid,ecm->sid,busycardid);


    if ( is_linked == 0 )
    {
        cc_disconnect_srv();
        runAsyncClient();
        is_linked = 1;
    }

	memcpy(&buffer_ecm_head[5],&ecm[3],25);
	
    buffer_ecm_head[5] = ecm[2];
    buffer_ecm_head[6] = ecm[3];
	buffer_ecm_head[3] = ecm[5];
    buffer_ecm_head[4] = ecm[6];
	
	

    uint32 provid = 0x00;

    uint16 caid = 0x4a02;
    uint16 sid = 0x04b8;
    uint16 ecmlen = len + 33;

    buf[0] = caid >> 8;
    buf[1] = caid & 0xff;
    buf[2] = provid >> 24;
    buf[3] = provid >> 16;
    buf[4] = provid >> 8;
    buf[5] = provid & 0xff;
    buf[6] = busycardid >> 24;
    buf[7] = busycardid >> 16;
    buf[8] = busycardid >> 8;
    buf[9] = busycardid & 0xff;
    buf[10] = sid >> 8;
    buf[11] = sid & 0xff;
    buf[12] = ecmlen;

    memcpy( &buf[13], &buffer_ecm_head[0], 33);
    memcpy( &buf[13 + 33], &ecm[0], len);

    // memcpy(&buf[13], eccm, 100);

    DBGmsg("CCcam: send ecm\r\n");

    if ( is_linked != 2 ) return 1;

    return cc_msg_send(  CC_MSG_ECM_REQUEST, 13 + ecmlen , buf );
}


