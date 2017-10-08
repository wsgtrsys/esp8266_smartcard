#ifndef SCLib_h
#define SCLib_h

#if defined(ARDUINO) && ARDUINO >= 100
  #include <Arduino.h>
#else
  #include "WProgram.h"
#endif

#include <ESP8266WiFi.h>
#include "cccam.h"

#include <Ticker.h>



// Default Config
#define DEFAULT_CLK_PIN              9
#define DEFAULT_ETU                372

// Maximum time of etu's to wait
#define MAX_WAIT_TIME            40000
// Maximum BYTES in ATR
#define MAX_ATR_BYTES 32


// APDU Command struct
typedef struct {
  /* CLASS byte of command */
  uint8_t  cla;
  /* INS byte of command */
  uint8_t  ins;
  /* Parameter 1 of command */
  uint8_t  p1;
  /* Parameter 2 of command */
  uint8_t  p2;
  /* pointer to data buffer containing "data_size" bytes 
     (Can be NULL to signal no data required) */
  uint8_t* data_buf;
  /* number of bytes in data_buf is ignored, if data_buf is NULL. 
     Can also be 0 to signal no data is required. */
  uint16_t data_size;
  /* max. number of bytes for response size */
  uint16_t resp_size;
} APDU_t;

class SmartCard
{
  public:
    SmartCard(uint8_t c7_io, uint8_t c2_rst, uint8_t c1_vcc, uint8_t card_present, uint8_t c3_clk, boolean card_present_invert=false);
    
	boolean send_atr();
	void card_run();
	void net_init();
	void rxRead(void);
	void clkRead(void);
	void time_callback(void);
	void setup(char *CCCam_server, uint16_t CCCam_port,char *CCCam_user,char *CCCam_pass,uint16_t POrt_Bits,char *cardid);
	void _timeOutoccured();

  private:
	size_t writee(uint8_t b);	
	void enableRx(bool on);

	void enableClock(bool on);
	boolean cardInserted();
    boolean timeout();
    uint16_t activate(uint8_t* buf, uint16_t buf_size);
    void deactivate();
    uint16_t getCurrentETU();
    void setGuardTime(unsigned long t);
    void ignoreParityErrors(boolean in);
	
	
	
	boolean get_cw();
	boolean send_ecm();
	void _init();
    
    uint8_t calcEDC(uint8_t startValue, uint8_t *buf, uint16_t size);
	boolean writeBytes(uint8_t* buf, uint16_t count);
    
    
	void hex_dump(uint8_t *buf, uint16_t len);

    
    uint8_t _io_in_pin;  // (INPUT/OUTPUT) : I/O pin to communication with Smart Card
    uint8_t _rstin_pin;  // (OUTPUT)       : LOW: reset active.  HIGH: inactive.
    uint8_t _cmdvcc_pin; // (OUTPUT)       : LOW: Activation sequence initiated HIGH: No activation in progress
    uint8_t _off_pin;    // (INPUT)        : HIGH: Card present LOW: No card present
    uint8_t _clk_pin;    // (OUTPUT)       : CLK signal for Smart Card (NOT_A_PIN, when no CLK generation)

    // Work Waiting Time (960 * D * Wi) etus
    unsigned long _wwt;
    // Maximum Waiting time (WWT + (D * 480)) etus
    unsigned long _max_wwt;
    
    boolean  _activated;
    boolean  _timeout;
    boolean  is_busy;
	boolean  ecm_head_send;
	// int   CW_geted;
	// Ticker ticker;
	Ticker cccam_ticker;
	unsigned long time_jiange;
	unsigned long ecm_jiange;

	unsigned long m_bitTime;
	unsigned long m_bitTime_07;
	unsigned int m_inPos, m_outPos;
   int m_buffSize;
   uint8_t m_buffer[128];
   uint8_t apdu[8];
   uint8_t card_local[8];
   uint8_t card_local_asc[8];
   
   int want_byte,sent_byte,ecm_len;
   uint8_t apdu_cmd;
   uint8_t ecm_data_buff[8];
   // uint8_t ecm_cw[16];
   // WiFiClient client;
   cccam cccam_client;
   
   // char CC_server[32];
   // uint16_t CC_port;   
   // char CC_user[32]; 
   // char CC_pass[32]; 
   uint16_t P_Bits;    
};

#endif
