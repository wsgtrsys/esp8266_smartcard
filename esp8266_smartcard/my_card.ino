#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <ESP8266WebServer.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <Ticker.h>
#include <EEPROM.h>
#include <ESPAsyncTCP.h>
#include <ESP8266HTTPUpdateServer.h>

#include "global.h"
#include "NTP.h"

// Include the HTML, STYLE and Script "Pages"

#include "Page_Admin.h"
#include "Page_Script.js.h"
#include "Page_Style.css.h"
#include "Page_NTPSettings.h"
#include "Page_Information.h"
#include "Page_General.h"
#include "Page_NetworkConfiguration.h"
#include "Page_CCCam.h"
#include "SCLib.h"

extern "C" {
#include "user_interface.h"
}


os_timer_t myTimer;


//*** Normal code definition here ...
//3029585 tx
//3571055 cqyx
#define LED_esp 			   2
#define SC_C2_RST              5
#define SC_C7_IO               2
#define SC_C2_CLK              12
#define SC_C1_VCC              11
#define SC_SWITCH_CARD_PRESENT 8
#define SC_SWITCH_CARD_PRESENT_INVERT false

String chipID;
SmartCard sc(SC_C7_IO, SC_C2_RST, SC_C1_VCC, SC_SWITCH_CARD_PRESENT, SC_C2_CLK, SC_SWITCH_CARD_PRESENT_INVERT);
void card_proc();
void flip();
bool card_reset;
bool cc_keeplive;

const char* update_path = "/firmware";
const char* update_username = "admin";
const char* update_password = "nimda";

ESP8266HTTPUpdateServer httpUpdater;



//AsyncClient * aClient = NULL;
//https://github.com/me-no-dev/ESPAsyncTCP/issues/18

void setup() {
  bool CFG_saved = false;
  int WIFI_connected = false;
  Serial.begin(115200);
  
 
  //**** Network Config load 
  EEPROM.begin(512); // define an EEPROM space of 512Bytes to store data
  CFG_saved = ReadConfig();

  //  Connect to WiFi acess point or start as Acess point
  if (CFG_saved)  //if no configuration yet saved, load defaults
  {    
      // Connect the ESP8266 to local WIFI network in Station mode
      Serial.println("Booting");
      //printConfig();
      WiFi.mode(WIFI_STA);  
      WiFi.begin(config.ssid.c_str(), config.password.c_str());
      WIFI_connected = WiFi.waitForConnectResult();
      if(WIFI_connected!= WL_CONNECTED )
          Serial.println("Connection Failed! activating to AP mode...");
      
      Serial.print("Wifi ip:");Serial.println(WiFi.localIP());
  }

  if ( (WIFI_connected!= WL_CONNECTED) or !CFG_saved){
	  
	  
	 if (! CFG_saved)
	{
    // DEFAULT CONFIG
    Serial.println("Setting AP mode default parameters");
    config.ssid = "OSCAM-" + String(ESP.getChipId(),HEX);       // SSID of access point
    config.password = "" ;   // password of access point
    config.dhcp = true;
    config.IP[0] = 192; config.IP[1] = 168; config.IP[2] = 100; config.IP[3] = 100;
    config.Netmask[0] = 255; config.Netmask[1] = 255; config.Netmask[2] = 255; config.Netmask[3] = 0;
    config.Gateway[0] = 192; config.Gateway[1] = 168; config.Gateway[2] = 100; config.Gateway[3] = 1;
    config.ntpServerName = "0.ch.pool.ntp.org"; // to be adjusted to PT ntp.ist.utl.pt
    config.Update_Time_Via_NTP_Every =  5;
    config.timeZone = 8;
    config.isDayLightSaving = true;
    config.DeviceName = "CCcam_dvb";
	 
    config.CCCam_server = "www.baidu.com";
    config.CCCam_port = 12000;
    config.CCCam_user = "test";
    config.CCCam_pass = "test";
    config.POrt_Bits = 9600;
    config.Card_id = "8523402123456789";
	
	 WiFi.mode(WIFI_AP);  
    WiFi.softAP(config.ssid.c_str());
    Serial.print("Wifi ip:");Serial.println(WiFi.softAPIP());
	}else
	{
		
		WiFi.mode(WIFI_AP);  
		WiFi.softAP("OSCAM-client");
		Serial.print("Wifi ip:");Serial.println(WiFi.softAPIP());			
	}   
     
   }
   

    // Start HTTP Server for configuration
    server.on ( "/", []() {
      // Serial.println("admin.html");
      server.send_P ( 200, "text/html", PAGE_AdminMainPage);  // const char top of page
    }  );
  
    server.on ( "/favicon.ico",   []() {
      // Serial.println("favicon.ico");
      server.send( 200, "text/html", "" );
    }  );
  
    // Network config
    server.on ( "/config.html", send_network_configuration_html );
    // Info Page
    server.on ( "/info.html", []() {
      // Serial.println("info.html");
      server.send_P ( 200, "text/html", PAGE_Information );
    }  );
    server.on ( "/ntp.html", send_NTP_configuration_html  );
  
    //server.on ( "/appl.html", send_application_configuration_html  );
    server.on ( "/general.html", send_general_html  );
    //  server.on ( "/example.html", []() { server.send_P ( 200, "text/html", PAGE_EXAMPLE );  } );
    server.on ( "/style.css", []() {
      // Serial.println("style.css");
      server.send_P ( 200, "text/plain", PAGE_Style_css );
    } );
    server.on ( "/microajax.js", []() {
      // Serial.println("microajax.js");
      server.send_P ( 200, "text/plain", PAGE_microajax_js );
    } );
    server.on ( "/admin/values", send_network_configuration_values_html );
    server.on ( "/admin/connectionstate", send_connection_state_values_html );
    server.on ( "/admin/infovalues", send_information_values_html );
    server.on ( "/admin/ntpvalues", send_NTP_configuration_values_html );
    //server.on ( "/admin/applvalues", send_application_configuration_values_html );
    server.on ( "/admin/generalvalues", send_general_configuration_values_html);
    server.on ( "/admin/devicename",     send_devicename_value_html);
    server.on ( "/admin/uptime",     send_information_uptime_html);
    
	server.on ( "/cccam.html",     send_CCCam_general_html);
    server.on ( "/admin/cccamvalues", send_CCCam_configuration_values_html);
  
  
    server.onNotFound ( []() {
      // Serial.println("Page Not Found");
      server.send ( 400, "text/html", "Page not Found" );
    }  );
	
	MDNS.begin(config.DeviceName.c_str());
	httpUpdater.setup(&server, update_path, update_username, update_password);
	 
    server.begin();
    Serial.println( "HTTP server started" );
	
	MDNS.addService("http", "tcp", 80);
	

    // ***********  OTA SETUP
  
    //ArduinoOTA.setHostname(host);
    // ArduinoOTA.onStart([]() { // what to do before OTA download insert code here
        // Serial.println("Start");
      // });
    // ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
      // Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    // });
    // ArduinoOTA.onEnd([]() { // do a fancy thing with our board led at end
        // for (int i=0;i<30;i++){
          // analogWrite(LED_esp,(i*100) % 1001);
          // delay(50);
        // }
        // digitalWrite(LED_esp,HIGH); // Switch OFF ESP LED to save energy
        // Serial.println("\nEnd");
        // ESP.restart();
      // });
  
    // ArduinoOTA.onError([](ota_error_t error) { 
        // Serial.printf("Error[%u]: ", error);
        // if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
        // else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
        // else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
        // else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
        // else if (error == OTA_END_ERROR) Serial.println("End Failed");
        // ESP.restart(); 
      // });
  
     // /* setup the OTA server */
    // ArduinoOTA.begin();
        
      
    // start internal time update ISR
    tkSecond.attach(1, ISRsecondTick);
		    
    //**** Normal Sketch code here... 
	sc.setup((char *)config.CCCam_server.c_str(), config.CCCam_port,(char *)config.CCCam_user.c_str(),(char *)config.CCCam_pass.c_str(),config.POrt_Bits,(char *)config.Card_id.c_str());	
	
	Serial.println("Ready");
	
	card_reset = false;
	cc_keeplive = false;
	pinMode(SC_C2_RST, INPUT); //INPUT_PULLUP   
    attachInterrupt(SC_C2_RST, card_proc, RISING);
	
}


void card_proc() {
	card_reset = true; 
}


  
// the loop function runs over and over again forever
void loop() {  
  // ArduinoOTA.handle();
  
  server.handleClient();
  customWatchdog = millis();  
  sc.card_run(); 
	
	if (card_reset) 
	{		
		sc.send_atr();
		card_reset = false;
	}
	
  //**** Normal Skecth code here ... 
}



