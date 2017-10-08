#include "SCLib.h"
#include <Arduino.h>
#include <inttypes.h>
#include <Stream.h>

extern "C" {
#include "gpio.h"
}

SmartCard *ObjList[3];

void ICACHE_RAM_ATTR sws_isr_0()
{
    ObjList[0]->rxRead();
};
void ICACHE_RAM_ATTR sws_isr_1()
{
    ObjList[0]->clkRead();
};

void ICACHE_RAM_ATTR sws_isr_2()
{
    ObjList[0]->time_callback();
};

// void ICACHE_RAM_ATTR sws_isr_3()
// {
    // ObjList[0]->_timeOutoccured();
// };

// #define DBGmsg Serial.printf
#define DBGmsg(c,...) do { ; } while(0)

uint8_t ecm_cw[17];


// STATES
// enum sync_state_t { SYNC_START_STATE, SYNC_START_CONDITION, SYNC_START_FINISHED, SYNC_CLK_LOW, SYNC_CLK_HIGH, SYNC_PRE_STOP_CONDITION, SYNC_STOP_CONDITION, SYNC_STOP_FINISHED, SYNC_FINISHED };
// enum state_t { START_STATE, FOUND_FIRST_FALLING_EDGE, FOUND_FIRST_RAISING_EDGE, SYNC_FOUND, READING_BITS, SENDING_BITS, PARITY_BIT, PARITY_ERROR, PARITY_READ, PRE_FINISHED, FINISHED };
// enum apdu_t0_state_t { SEND_HEADER, WAIT_ANSWER, SEND_DATA, RECEIVE_DATA, SEND_RESPONSE_SIZE, ADPU_FINISHED };

/******************************************************************************
 * Constructors
 ******************************************************************************/
/**
 * c7_io               - IO line, connected to Card (C7)
 * c2_rst              - RST for Card (C2)
 * c1_vcc              - Vcc for Card (C1)
 * card_present        - Card present - HIGH - Card available - LOW - Card removed (Can be changed by card_present_invert)
 * c3_clk              - clk signal to SC (C3) - (timer1 / pin9 used)
 * card_present_invert - Use inverted off signal (LOW Card available - HIGH Card removed)
 */
SmartCard::SmartCard(uint8_t c7_io, uint8_t c2_rst, uint8_t c1_vcc, uint8_t card_present, uint8_t c3_clk, boolean card_present_invert)
{
    _io_in_pin  = c7_io;
    _rstin_pin  = c2_rst;
    _cmdvcc_pin = c1_vcc;
    _off_pin    = card_present;
    _clk_pin    = c3_clk;

    ObjList[0] = this;
}


void SmartCard::setup(char *CCCam_server, uint16_t CCCam_port, char *CCCam_user, char *CCCam_pass, uint16_t POrt_Bits,char *cardid)
{
    uint16_t card_local_int; 
	
	cccam_client.Setup(CCCam_server, CCCam_port, CCCam_user, CCCam_pass);
	memcpy(card_local,cardid,7);
	memset(card_local_asc,0,8);
	memcpy(card_local_asc,card_local+1,4);
	card_local_int = atoi((const char *)card_local_asc);	
	card_local_asc[0] = (card_local_int>>8) & 0xFF;
	card_local_asc[1] = (card_local_int) & 0xFF;
    P_Bits =  POrt_Bits;
	cccam_ticker.attach(30, sws_isr_2);//添加一个定时器
	_init();
	enableRx(true);
	
}


void SmartCard::_init()
{

    pinMode(_io_in_pin, OUTPUT);//OUTPUT
    digitalWrite(_io_in_pin, HIGH);//LOW

    // pinMode(_rstin_pin, INPUT);

    m_bitTime = ESP.getCpuFreqMHz() * 1000000 / P_Bits;//8144 9600
    m_bitTime_07 = m_bitTime * 0.6;

    m_buffSize = 128;
    m_inPos = 0;
    want_byte = 0;
    is_busy = false;
}

// void SmartCard::net_init()
// {
// Serial.print("connecting to cccam server\r\n");
// // Serial.println(host);

// const int httpPort = 44444;
// if (!client.connect("10.18.5.51", httpPort))
// {
// Serial.println("connection cccam server failed\r\n");
// return;
// }
// else
// {
// Serial.println("connection cccam server fine\r\n");
// }

// }


// boolean SmartCard::send_ecm()
// {
// uint16_t ret, i = 0;
// uint8_t buffer[128];
// uint8_t buffer_ecm_head[] =
// {
// 0x80, 0x70, 0x61, 0x31, 0x91, 0x08, 0xFC, 0x00, 0x0E, 0x16, 0x1E, 0x00, 0x12, 0x2F, 0x75, 0x26,
// 0xBC, 0x63, 0x91, 0x0F, 0x04, 0x49, 0x04, 0x7F, 0x6D, 0x22, 0xA5, 0x4A, 0x91, 0x2F, 0x44, 0x12,
// 0x43
// };

// buffer[0] = 0x02;// ecm包
// buffer[1] = 0x4a; // CAI ID
// buffer[2] = 0x02; // CAI ID

// buffer[3] = 0x66; // svrId ID
// buffer[4] = 0x66; // svrId ID

// // buffer[5] = (0xff & sent_byte);// 序列号
// // buffer[6] = ((0xff00 & sent_byte) >> 8);
// // buffer[7] = ((0xff0000 & sent_byte) >> 16);
// // buffer[8] = ((0xff000000 & sent_byte) >> 24);

// if (is_busy == true) return false;
// if (ecm_len > 70 ) return false;

// buffer_ecm_head[5] = m_buffer[2];
// buffer_ecm_head[6] = m_buffer[3];
// memcpy(buffer + 9, buffer_ecm_head, 33);
// memcpy(buffer + 9 + 33 , m_buffer, ecm_len);

// if (client.connected())
// {
// ret = client.write((const uint8_t *)buffer, ecm_len + 9 + 33);
// if (ret != ecm_len + 9 + 33)
// {
// Serial.println("sendto cccam failed\r\n");
// return false;
// }
// is_busy = true;
// }
// else
// {
// client.stop();
// net_init();
// if (client.connected())
// {
// ret = client.write((const uint8_t *)buffer, ecm_len + 9 + 33);
// if (ret != ecm_len + 9 + 33)
// {
// Serial.println("sendto cccam failed\r\n");
// return false;
// }
// is_busy = true;
// }
// }
// }


// boolean SmartCard::get_cw()
// {
// uint8_t buffer[128];
// uint16_t i = 0;
// unsigned long timeout = millis();

// is_busy = false;

// if (client.connected())
// {
// while (client.available() == 0)
// {
// if (millis() - timeout > 700)
// {
// Serial.println(">>> oscam server Timeout !");
// // client.stop();
// return false;
// }
// }

// while(client.available())
// {
// if (i < 128) buffer[i++] = client.read(); //直接收到m_buffer中
// }

// memcpy(m_buffer+5, buffer + 5, 16);
// return true;
// }

// return false;
// }


uint8_t SmartCard::calcEDC(uint8_t startValue, uint8_t *buf, uint16_t size)
{
    uint8_t xorvalue = startValue;
    for(size_t i = 0; i < size; i++)
    {
        xorvalue ^= buf[i];
    }
    return xorvalue;
}

boolean SmartCard::cardInserted()
{
    // if (_off_invert)
    // return (digitalRead(_off_pin) == LOW);
    // return (digitalRead(_off_pin) == HIGH);
    // return (digitalRead(_rstin_pin) == HIGH);
    // attachInterrupt(_rstin_pin, send_atr, _off_invert ? RISING : FALLING);
    return true;
}


boolean SmartCard::writeBytes(uint8_t *buf, uint16_t count)
{
    boolean parityError = false;
    if (buf != NULL && count > 0)
    {
        for(uint16_t i = 0; i < count; i++)
        {
            writee(buf[i]);
            // delayMicroseconds(15);
        }
    }
    return parityError;
}

void SmartCard::deactivate()
{
    _timeout = false;
}


#define WAIT { while (ESP.getCycleCount()-start < wait); wait += m_bitTime; }
#define WAIT1 { while (ESP.getCycleCount()-start < wait); wait += m_bitTime_07; }

size_t SmartCard::writee(uint8_t b)
{

    boolean currentBit = false;
    boolean parity = false;
    boolean parityErrorSignaled = false;
    unsigned long start1 = ESP.getCycleCount();

    // DBGmsg("\r\nsend data %d\r\n",micros());
    // delay(5);
    cli();

    unsigned long wait = m_bitTime;

    pinMode(_io_in_pin, OUTPUT);
    digitalWrite(_io_in_pin, HIGH);
    while (ESP.getCycleCount() - start1 < (m_bitTime +m_bitTime));

    unsigned long start = ESP.getCycleCount() + 18;
    // Start bit;
    digitalWrite(_io_in_pin, LOW);//36个指令周期
    WAIT;

    for (int i = 0; i < 8; i++)
    {
        currentBit =   b & 1;
        digitalWrite(_io_in_pin, currentBit ? HIGH : LOW);
        if (currentBit) parity = !parity;//校验
        WAIT;
        b >>= 1;
    }//371个指令周期,不含wait

    // PARITY_BIT  校验
    digitalWrite(_io_in_pin, ((parity) ? HIGH : LOW));
    WAIT;


    digitalWrite(_io_in_pin, HIGH);
    WAIT
    pinMode(_io_in_pin, INPUT);
    // WAIT1;//等待半个etu


    // while (digitalRead(_io_in_pin) == LOW);
    // DBGmsg("\r\nsend data %d\r\n",m_bitTime);
    parityErrorSignaled = (digitalRead(_io_in_pin) == HIGH);

    // DBGmsg("\r\nsend data %d\r\n",parityErrorSignaled);

    sei();
    return 1;
}

void SmartCard::rxRead(void)
{
    unsigned long wait = m_bitTime  ;
    pinMode(_io_in_pin, INPUT);

    unsigned long start = ESP.getCycleCount();
    uint8_t rec = 0;
    boolean parity = true;

    WAIT1

    for (int i = 0; i < 8; i++)
    {
        WAIT;
        rec  >>= 1;
        if (digitalRead(_io_in_pin) != LOW)
        {
            rec  |= 0x80;
            parity = !parity;
        }
        else
        {
            rec &= 0x7F;
            // DBGmsg("n_bitTime=%d %x\r\n",micros(),ESP.getCycleCount());
        }
    }

    // digitalRead(_io_in_pin);
    pinMode(_io_in_pin, OUTPUT);
    digitalWrite(_io_in_pin, HIGH);
    WAIT

    pinMode(_io_in_pin, INPUT);
    WAIT
    //WAIT1


    int next = (m_inPos + 1) % m_buffSize;
    if (next != m_inPos)
    {
        m_buffer[m_inPos] = rec;
        m_inPos = next;
    }

    GPIO_REG_WRITE(GPIO_STATUS_W1TC_ADDRESS, 1 << _io_in_pin);
}

void SmartCard::clkRead(void)
{
    DBGmsg("Clock_bitTime=%d %x\r\n", micros(), ESP.getCycleCount());

    // countt++;

    // if (countt > 16)

    enableClock(false);

    GPIO_REG_WRITE(GPIO_STATUS_W1TC_ADDRESS, 1 << _clk_pin);
}


void SmartCard::enableRx(bool on)
{

    if (on)
    {
        pinMode(_io_in_pin, INPUT);
        attachInterrupt(_io_in_pin,  sws_isr_0,  FALLING );
    }
    else
        detachInterrupt(_io_in_pin);
}

void SmartCard::enableClock(bool on)
{

    if (on)
    {
        pinMode(_clk_pin, INPUT);
        attachInterrupt(_clk_pin,  sws_isr_1,  RISING );
    }

    else
        detachInterrupt(_clk_pin);
}

void SmartCard::hex_dump(uint8_t *buf, uint16_t len)
{
    uint16_t i;

    for (i = 0; i < len; i++)
    {
        if (i && !(i % 16))
            DBGmsg("\r\n");
        DBGmsg("%02x ", *(buf + i));
    }
    DBGmsg("\r\n");
}

boolean SmartCard::send_atr()
{

    DBGmsg("send atr\r\n");

    _init();


    uint8_t data[32];
    data[0] = 0x3b;
    data[1] = 0x6c;
    data[2] = 0x00;
    data[3] = 0x00;
    data[4] = 0x4e;
    data[5] = 0x54;
    data[6] = 0x49;
    data[7] = 0x43;
    data[8] = 0x31;
    data[9] = 0x0a;
    data[10] = 0x88;
    data[11] = 0x14;
    data[12] = 0x4a;
    data[13] = 0x03;
    data[14] = 0x00;
    data[15] = 0x00;


    // enableRx(false);
    // delay(1);
    writeBytes(data, 16);

    // enableClock(true);
    // enableRx(true);

    // cccam_client.send_ecm( m_buffer, 67);
    // cccam_client.get_cw( m_buffer);

}


void SmartCard::card_run()
{
    unsigned long start1 = ESP.getCycleCount();
	


    uint8_t begin_cmd2[] = {0x00, 0xa4, 0x04, 0x00, 0x05, 0xf9, 0x5a, 0x54, 0x00, 0x06};
    // uint8_t get_serial_cmd[] = {0x80, 0x46, 0x00, 0x00, 0x04, 0x01, 0x00, 0x00, 0x04};
    // uint8_t pairing_cmd[] = {0x80, 0x4c, 0x00, 0x00, 0x04, 0xFF, 0xFF, 0xFF, 0xFF};
    // uint8_t ok_cmd[] = {0x90, 0x00};
    // uint8_t step_cmd[] = {0x61, 0x04};//这里要修改返回的长度
    uint8_t read_data_cmd[] = {0x00, 0xc0, 0x00, 0x00};

    // uint8_t get_provider_cmd[] = {0x80, 0x44, 0x00, 0x00, 0x08, 0x08, 0xFC, 0x00, 0x00, 0x00, 0x0A, 0x00, 0x00};
    // uint8_t yxtf3_get_subscription[] = {0x80, 0x48, 0x00, 0x01, 0x04, 0x01, 0x00, 0x00, 0x13};
    uint8_t ecm_cmd[] = {0x80, 0x3a};
    uint8_t ecm_data[] =
    {
        0x0a, 0x08, 0xfc, 0x00, 0x16, 0x1f, 0x70, 0xef,
        0xe8, 0xa5, 0x0c, 0x99, 0x28, 0x84, 0x3f, 0xeb,
        0x0e, 0x0e, 0x1c, 0x38, 0xb8, 0xb9, 0x04, 0x75,
        0x00, 0x0c, 0x5e, 0x00, 0x01, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x90, 0x00
    };


    uint8_t cmd1[] = {0x84, 0xAA, 0x00, 0x00, 0x30};
    uint8_t cmd1r[] = {0x6D, 0x00};
    uint8_t cmd2[] = {0x80, 0x46, 0x00, 0x00, 0x04, 0x07, 0x00, 0x00, 0x08};
    uint8_t cmd2r[] = {0x61, 0x08};
    uint8_t cmd2rr[] = {0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x90 , 0x00};
    uint8_t cmd4[] = {0x00, 0xA4, 0x04, 0x00, 0x05, 0xF9, 0x5A, 0x54, 0x00, 0x06};
    uint8_t cmd5[] = {0x80, 0x46, 0x00, 0x00, 0x04, 0x01, 0x00, 0x00, 0x14};
	//前面四个字节的十进制值，约等于后面文本串的值。其中082300中的，23，应该就是区域代码
    uint8_t cmd5r[] = {0x06, 0x95, 0x55, 0x76, 0x38, 0x35, 0x33, 0x31, 0x31, 0x30, 0x33, 0x31, 0x30, 0x34, 0x35, 0x30, 0x30, 0x33, 0x38, 0x38, 0x90, 0x00};//
    uint8_t cmd6[] = {0x80, 0x46, 0x00, 0x00, 0x04, 0x03, 0x00, 0x00, 0x09};
    uint8_t cmd6r[] = {0x0F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x90, 0x00};
    uint8_t cmd7[] = {0x80, 0x44, 0x00, 0x00, 0x08};
    uint8_t cmd7r[] = {0x08, 0xFC, 0x00, 0x0A, 0x00, 0x00, 0x00, 0x00};

    uint8_t cmd8[] = {0x80, 0x4C, 0x00, 0x00, 0x04, 0xFF, 0xFF, 0xFF, 0xFF};
    uint8_t cmd9[] = { 0x80, 0x46, 0x08, 0xFC, 0x04, 0x04, 0x00, 0x00, 0x48};
    // uint8_t cmd9r[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x62, 0xE4, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x90, 0x00};
    uint8_t cmd9r[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x90, 0x00};
	//cmd9r 中的62e4,转为10进制就是25316，就是用户类型
    uint8_t cmd10[] = {0x80, 0x48, 0x08, 0xFC, 0x04, 0x81, 0x00, 0x00, 0x3E};
    // uint8_t cmd10r[] = {0x00, 0x00, 0x3D, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x80, 0x70, 0x00, 0x01, 0xFF, 0xFE, 0x00, 0x01, 0x00, 0x73, 0x00, 0x01, 0x00, 0x76, 0x00, 0x01, 0x00, 0x74, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x63, 0x00, 0x01, 0x00, 0x6A, 0x00, 0x01, 0x00, 0x69, 0x00, 0x01, 0x00, 0x20, 0x00, 0x01, 0x00, 0x19, 0x00, 0x01, 0x00, 0x40, 0x00, 0x01, 0x00, 0x35, 0x00, 0x01, 0x00, 0x1C, 0x00, 0x01, 0x00, 0x29, 0x00, 0x01, 0x00, 0x38, 0x00, 0x01, 0x00, 0x47, 0x00, 0x01, 0x00, 0x44, 0x00, 0x01, 0x00, 0x51, 0x00, 0x01, 0x00, 0x4F, 0x00, 0x01, 0x00, 0x37, 0x00, 0x01, 0x00, 0x07, 0x00, 0x01, 0x00, 0x49, 0x00, 0x01, 0x00, 0x2B, 0x00, 0x01, 0x00, 0x42, 0x00, 0x01, 0x00, 0x5D, 0x00, 0x01, 0x00, 0x5C, 0x00, 0x01, 0x00, 0x5A, 0x00, 0x01, 0x00, 0x59, 0x00, 0x01, 0x00, 0x58, 0x00, 0x01, 0x00, 0x57, 0x00, 0x01, 0x00, 0x56, 0x00, 0x01, 0x00, 0x61, 0x00, 0x01, 0x00, 0x3F, 0x00, 0x01, 0x00, 0x41, 0x00, 0x01, 0x00, 0x4E, 0x00, 0x01, 0x00, 0x1D, 0x00, 0x01, 0x00, 0x1E, 0x00, 0x01, 0x00, 0x2F, 0x00, 0x01, 0x00, 0x4A, 0x00, 0x01, 0x00, 0x4C, 0x00, 0x01, 0x00, 0x53, 0x00, 0x01, 0x00, 0x4D, 0x00, 0x01, 0x00, 0x3D, 0x00, 0x01, 0x00, 0x25, 0x00, 0x01, 0x00, 0x1B, 0x00, 0x01, 0x00, 0x1A, 0x00, 0x01, 0x00, 0x08, 0x00, 0x01, 0x00, 0x48, 0x00, 0x01, 0x00, 0x26, 0x00, 0x01, 0x00, 0x3B, 0x00, 0x01, 0x00, 0x3E, 0x00, 0x01, 0x00, 0x50, 0x00, 0x01, 0x00, 0x3A, 0x00, 0x01, 0x00, 0x06, 0x00, 0x01, 0x00, 0x77, 0x90, 0x00};
    uint8_t cmd10r[] = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x90,0x00};
    uint8_t cmd11[] = {0x80, 0x46, 0x08, 0xFC, 0x04, 0x02, 0x00, 0x00, 0x4F};
    // uint8_t cmd11r[] = {0x08, 0xFC, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x17, 0xFB, 0x90, 0x10, 0x00, 0x3A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x90, 0x00};
    uint8_t cmd11r[] = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x90,0x00};
    uint8_t cmd12[] = {0x80, 0x4E, 0x08, 0xFC, 0x11,};
    uint8_t cmd13[] = {0x80, 0x40, 0x08, 0xFC, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    uint8_t cmd14[] = {0x80, 0x46, 0x00, 0x00, 0x04, 0x05, 0x00, 0x00, 0x04};


    if ( m_inPos == 5 && want_byte == 0 )
    {
        // while (ESP.getCycleCount() - start1 < 25000);
		// yield();
        if (memcmp(read_data_cmd, m_buffer, 4) == 0  ) //发送回二次请求数据
        {
            if (apdu_cmd == 11)
            {
				
				if ( ! ecm_head_send)
				{
					sent_byte = m_buffer[4];
					writee(m_buffer[1]); 
					ecm_head_send = true;
					DBGmsg("recv data get cmd:\r\n");
				}
				
				if ( ( millis() - time_jiange ) > 20500 || ( millis() - ecm_jiange ) > 600  ) 
						_timeout = true;
					else
						_timeout = false;
				
                if (ecm_cw[16] == 0x01 || _timeout )//有数据回来再发
                {
                    memcpy(ecm_data, ecm_data_buff, 8);
					
					if ( _timeout )
					{						
							memcpy(ecm_data + 8, ecm_cw+8, 8);
							
					}		
					else
						{
							memcpy(ecm_data + 8, ecm_cw, 16);
							hex_dump(ecm_cw,16);
						}
						
						
                    ecm_cw[16] = 0;
					m_inPos = 0;
					writeBytes(ecm_data, sent_byte+2);
					time_jiange = millis();
					DBGmsg("send ecm data:\r\n");
                }
            }
            else
            {
                want_byte = 0;
                m_inPos = 0;
                sent_byte = m_buffer[4];
                writee(m_buffer[1]);
                switch (apdu_cmd )
                {
                case 1://cmd2
                    writeBytes(cmd2rr, sent_byte);
                    break;
                case 2://cmd5r
					memcpy(cmd5r+4 , card_local, 7);
                    writeBytes(cmd5r, sent_byte);
                    break;
                case 3://cmd6r
                    writeBytes(cmd6r, sent_byte);
                    break;
                case 6:
                    writeBytes(cmd9r, sent_byte);
                    break;
                case 7:
					// memcpy(cmd10r,card_local_asc+2,2);
                    writeBytes(cmd10r, sent_byte);
                    break;
                case 8:
					// memcpy(cmd11r,card_local_asc+2,2);
                    writeBytes(cmd11r, sent_byte);					
                    break;
                case 11://ecm
                    // memcpy(ecm_data, ecm_data_buff, 8);
                    // // get_cw();
                    // if (CW_geted == 1 )
                    // {
                        // memcpy(ecm_data + 8, ecm_cw, 16);
                        // CW_geted = 0;
                    // }
                    // else
                    // {
                        // cccam_client.get_cw( ecm_cw );
                        // memcpy(ecm_data + 8, ecm_cw, 16);
                    // }
                    // writeBytes(ecm_data, sent_byte);
                    break;
                case 12://
                    writeBytes(ecm_data, sent_byte);
                    break;
                }
                writee(0x90);
                writee(0x00);
                // DBGmsg("recv data get cmd:\r\n");
                hex_dump(m_buffer, 5);
            }
        }
        else if (memcmp(m_buffer, cmd7, 5) == 0 )//get_provider_cmd cmd7 收到命令头直接返回
        {
            writee(m_buffer[1]);
			memcpy(cmd7r,card_local_asc,2);
            writeBytes(cmd7r, 8);
            writee(0x90);
            writee(0x00);
            want_byte = 0;
            m_inPos = 0;
            DBGmsg("recv head:\r\n");
            hex_dump(m_buffer, 5);
        }
        else if (memcmp(m_buffer, cmd12, 2) == 0  && m_buffer[4] == 0x11 ) //cmd12
        {
            writee(0x94);
            writee(0x92);
            want_byte = 0;
            m_inPos = 0;
            DBGmsg("recv head:\r\n");
            hex_dump(m_buffer, 5);
        }
        else
        {
            writee(m_buffer[1]);
            want_byte = m_buffer[4];
            DBGmsg("send guocen:%x\r\n",m_buffer[1]);
            // hex_dump(m_buffer, 5);
        }
    }



    if ( m_inPos - 5 == want_byte && want_byte != 0 )
    {
        // while (ESP.getCycleCount() - start1 < 25000);

        if (memcmp(m_buffer, begin_cmd2, 6) == 0 )//开始命令 cmd4
        {
            writee(0x90);
            writee(0x00);
        }
        else if (memcmp(m_buffer, cmd2, 6) == 0  )// cmd2
        {
            writee(0x61);
            writee(0x08);
            apdu_cmd = 1;
        }
        else if (memcmp(m_buffer, cmd5, 6) == 0  )  // cmd5
        {
            writee(0x61);
            writee(0x14);
            apdu_cmd = 2;
        }
        else if (memcmp(m_buffer, cmd6, 6) == 0  )  // cmd6
        {
            writee(0x61);
            writee(0x09);
            apdu_cmd = 3;
        }
        else if (memcmp(m_buffer, cmd8, 5) == 0 )//pairing_cmd cmd8
        {
            writee(0x94);
            writee(0xb1);
            apdu_cmd = 5;
        }
        else if (memcmp(m_buffer, cmd9, 2) == 0 && m_buffer[4] == 0x04 && m_buffer[5] == 0x04 )// cmd9
        {
            writee(0x61);
            writee(0x48);
            apdu_cmd = 6;
        }
        else if (memcmp(m_buffer, cmd10, 2) == 0 && m_buffer[4] == 0x04 ) //yxtf3_get_subscription 获取授权列表 cmd10
        {            
			writee(0x61);
            writee(0x0b);
			memcpy(card_local_asc+2,m_buffer+2,2);
            apdu_cmd = 7;
        }
        else if (memcmp(m_buffer, cmd11, 2) == 0 && m_buffer[4] == 0x04 && m_buffer[5] == 0x02) //cmd11
        {            
			writee(0x61);
            writee(0x2d);
			memcpy(card_local_asc+2,m_buffer+2,2);			
            apdu_cmd = 8;
        }

        else if (memcmp(m_buffer, cmd13, 2) == 0  && m_buffer[4] == 0x06  ) //cmd13
        {
            writee(0x90);
            writee(0x00);
            apdu_cmd = 10;
        }

        else if (memcmp(m_buffer, ecm_cmd, 2) == 0  )//ecm数据
        {
            ecm_len = m_inPos;
            ecm_data_buff[0] = 0x0a;
            ecm_data_buff[1] = m_buffer[2];
            ecm_data_buff[2] = m_buffer[3];
            ecm_data_buff[3] = m_buffer[8];
            ecm_data_buff[4] = m_buffer[9];
            ecm_data_buff[5] = m_buffer[10];
            ecm_data_buff[6] = m_buffer[11];
            ecm_data_buff[7] = m_buffer[12];
            cccam_client.send_ecm( m_buffer, ecm_len);
			// ecm_cw[16] = 0x00;
			// yield();
            // delay(100);
			// ticker.attach_ms(600, sws_isr_3);//添加一个定时器
			ecm_jiange = millis();
            // send_ecm();
            writee(0x61);
            writee(0x2b);
            apdu_cmd = 11;
			ecm_head_send = false;
        }
        else if (memcmp(m_buffer, cmd14, 2) == 0 && m_buffer[4] == 0x04  && m_buffer[5] == 0x05) //ecm数据
        {
            writee(0x61);
            writee(0x04);
            apdu_cmd = 12;
        }
        else if (memcmp(m_buffer, cmd1, 5) == 0 )
        {
            writee(0x90);
            writee(0x00);
            apdu_cmd = 12;
        }
        else if ( m_buffer[0] == 0x80  && m_buffer[1] == 0x32 )
        {
            writee(0x90);
            writee(0x00);
            DBGmsg("recv EMM:\r\n");
        }

        DBGmsg("recv data:\r\n");
        // hex_dump(m_buffer, m_inPos > 16 ? 16 : m_inPos);
        want_byte = 0;
        m_inPos = 0;
    }

}


void SmartCard::time_callback(void)
{
    DBGmsg("time_callback Keepalive\r\n");
    cccam_client.Keepalive();
    ;
}


void SmartCard::_timeOutoccured()
{
    _timeout = true;
	// ticker.detach();//去掉
}

//https://github.com/3PO/vdr-plugin-sc/blob/master/systems/cardclient/cccam2.c
// http://lolikitty.pixnet.net/blog/post/168715383-arduino-入門教學：timer-使用
// https://github.com/gravatasufoca/Twin2cc/blob/master/cccam-srv.c