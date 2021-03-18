#ifndef _LET_ESP32_
#define _LET_ESP32_

// let-esp32: a lightweight event tracebuffer for the TinyPico board
// uses websockets to transfer the eventtrace data to a webserver
// where it can be rendered with a javascript frontend
//
// requires: ArduinoWebsockets
//
// author: stf

#include <ArduinoWebsockets.h>
#include <WiFi.h>

#define LETESP32_BUFF_DEPTH 256 // record 4K events before transfering them 

using namespace websockets;

#pragma pack(1)
struct event_t {
	uint64_t t;
	uint32_t id;
};	
#pragma pop(1)

// The buffer type with the recorded data (binary protocol)
#pragma pack(1)
struct buffer_t {
        char ident[8]; // dotDevice identifiers are 8 chars -- 64bits
        uint32_t cmd;
        event_t buff[LETESP32_BUFF_DEPTH];
};
#pragma pop(1)


// display callback event from websocket connection
void letesp32_onEventsCallback(WebsocketsEvent event, String data) {
    if(event == WebsocketsEvent::ConnectionOpened) {
        Serial.println("Connnection Opened");
    } else if(event == WebsocketsEvent::ConnectionClosed) {
        Serial.println("Connnection Closed");
    } else if(event == WebsocketsEvent::GotPing) {
        Serial.println("Got a Ping!");
    } else if(event == WebsocketsEvent::GotPong) {
        Serial.println("Got a Pong!");
    }
}

class LetESP32
{
    public:
	    LetESP32(const char* ssid, const char* password, const char *ws, const char* ident){
		    _ws = ws;
		    _ssid = ssid;
		    _password = password;
            strcpy(_trace.ident, ident); // unique identifier for this device
            _time_last_pulse = millis();

		    // initialise the hardware event timer
		    initTimer0();
	    } 

	    void connect(){
   		    WiFi.begin(_ssid, _password);
   		    Serial.begin(115200);

   		    for(int i=0; i < 10 && WiFi.status() != WL_CONNECTED; i++) {
   		            delay(1000);
   		    }   
   		    Serial.println("Connected");

   		    _client.onEvent(letesp32_onEventsCallback);
   		    _client.connect(_ws);

   		    Serial.println("Connected to websocket");
   		    Serial.flush();

   		    _client.ping();
	    }

            // Initialise one of the counters
        void initTimer0() {
            
                 uint32_t* alarm1 = (uint32_t *)((void *)0x3FF5F010);    
                 uint32_t* alarm2 = (uint32_t *)((void *)0x3FF5F014);
                 *alarm1 = 0xFFFFFFFF;
                 *alarm2 = 0xFFFFFFFF;

                 // Set the Load value -- starts the timer from 0
                 uint32_t* loadlo = (uint32_t *)(0x3FF5F018);
                 uint32_t* loadhi = (uint32_t *)(0x3FF5F01C);
                 *loadlo = 0;
                 *loadhi = 0;

                 // Configuration
                 uint32_t *config = (uint32_t *)((void *)0x3FF5F000);
                 uint32_t d = 0;
                 *config = d; // clear the config
                 d |= (0x1 << 31) | (0x1 << 30) | (0x2 << 13);
                 //d |= (0x50 << 13); // divide by 80 gives us a usec timer
                 *config = d;


		         resetTimer0();

                 return;
        }

	    void resetTimer0(){
                 // Reload the timer with 0
                 uint32_t *reload = (uint32_t *)(0x3FF5F020);
                 *reload = 1;
		         return;
	    }

	    // Reads the timer value
        uint64_t readTimer0(){
                  // Trigger the timer to copy the value to the register
                  *_timertrig = 1;
                  uint32_t *lowerbits = (uint32_t *)((void *)0x3FF5F004);
                  uint32_t *upperbits = (uint32_t *)((void *)0x3FF5F008);
                  uint64_t ut = *upperbits;
                  uint64_t t = (ut << 32) | *lowerbits;
                  resetTimer0();
                  return t;
            }

	    void event(uint16_t id) { // register an event with an id
                noInterrupts();
		        _trace.buff[_bufpos].t = readTimer0();
                _trace.buff[_bufpos].id = id;
                _bufpos = _bufpos + 1;
                interrupts();
                if(_bufpos >= LETESP32_BUFF_DEPTH) // we are at the end of the buffer
	                flushLET();		
	    }

        void sendJSON(String &str) {
            // block til we can send
            while(_time_last_transfer + _rate_limit > millis()) { }
            _client.send(str);
            _time_last_transfer = millis();
        }

        void sendJSON(char* str) {
            while(_time_last_transfer + _rate_limit > millis()) { }
            _client.send(str);
            _time_last_transfer = millis();
        }

        void sendBIN(char *t) {
            while(_time_last_transfer + _rate_limit > millis()) { }
            _client.sendBinary(t, 76);
            _time_last_transfer = millis();
        }

	    void flushLET() {
	        _bufpos = 0;	
            if(_time_last_pulse + _rate_limit < millis()) {
                 _client.sendBinary((char *)(& _trace), sizeof(buffer_t));
                 _time_last_pulse = millis();
            }
		    resetTimer0();
	    } 

    private: 
	   const char * _ssid;
	   const char * _password;
	   const char * _ws;
	   uint32_t _bufpos;
       buffer_t _trace;
	   WebsocketsClient _client;
       unsigned long _time_last_pulse;
       unsigned long _time_last_transfer;
       const unsigned long _rate_limit = 2000; // need to wait at least 2 seconds between flushes

 	   // Timer hardware registers
       uint32_t *_timertrig = (uint32_t *)((void*)0x3FF5F00C);

};

#endif /* _LET_ESP32_ */
