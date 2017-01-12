#include <Arduino.h>

/*
  IP Remote hardware Switch for Arduino rev.0.3
-------------------------------------------------
  http://RemoteQTH.com/ip-switch.php
  2016-12 by OK1HRA
  
  ___               _        ___ _____ _  _                
 | _ \___ _ __  ___| |_ ___ / _ \_   _| || |  __ ___ _ __  
 |   / -_) '  \/ _ \  _/ -_) (_) || | | __ |_/ _/ _ \ '  \ 
 |_|_\___|_|_|_\___/\__\___|\__\_\|_| |_||_(_)__\___/_|_|_|
                                                           

More features:
- from 1 to 11 outputs, protect to only single out switch ON (antenna switching) - from rev 0.2 maybe off
- network latency (delay) shows OFF time linked LED
- relay parking to define output (default 11), after network unavailable with preset delay
- CFG button reconfigure chip and redirect network
  this allows one control unit for up to 11 relay board (176 relay outputs max)

Changelog:
2016-12 - 
2016-07 - new settings fullBits
        - pin 13 ON for TCP232 cfg pin

Settings:
*////////////////////////////////////////////////////////////////////////////////
//#define CONTROL         //      [Control/Relay] uncomment for CONTROL
//#define controlCFG    //      [Control]       uncoment for enable reconfigurable network - need set configure array (for relay not implemented yet)
//#define fullBits      //      data1 (PIND and PINB) disable one from 11 filter = use full range bits - MUST be sets same on both side
#define delayC   1000   //  ms  [Control]       time, are waiting for answers
                        //                      after timeout linked led OFF, and resend data
#define delayR   10     //  sec [Relay]         determines the time, after which the relay parking
#define parking  11     //  out [Relay]  1-11   parking output after timeout - optimally for grounded antenna
/////////////////////////////////////////////////////////////////////////////////
/*
                     Arduino Ports / in-out layout
-------------------------------------------------------------------------------
        PIND       |       PINB        |         PINC         | Arduino Ports
   6 5 4 3 2 1 x x | x x x 11 10 9 8 7 | x x x 16 15 14 13 12 | switch number
               d a t a 1               |    data2|   data3    | data transfer
-------------------------------------------------------------------------------
  format data - three cvs number
   X,X,X\n           range
   | | |____  data3  0-15
   | |______  data2  0-1  [0-3 with verify free bit]
   |________  data1  1-11

                          Data Flow
                        -------------
    control                                        relay
   ---------                                      -------
   --->|                                                        - read inputs
      xxx -------------------->-------------------> xxx-->      - tx > rx and set outputs
                                                     |          - 
       f  <-------------------<--------------------  f          - verify (keep alive)
       |
       |                                                        - waiting for timeout or change input
*/
int data1; // 1-11 switch (4bit)
#if defined(fullBits)
  int data1b;
#endif
int data2; // 16 switch (1bit)
int data3; // 12-15 switch (4bit)
int previous;
int timeout;
char ch = 0;
int baud = 9600;
#if defined(CONTROL) //----------------------------------- control
      #define CFG A6   // CFG SW - control only
      int cache1 = 0;
      int cache1b = 0;
      int cache2 = 0;
      int cache3 = 0;
      int data2b = 0;
      int data1aa = 0;
      int data1bb = 0;
      #if defined(controlCFG)
          byte CFGmsgC[11][31] = {
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//          Configuration string for eleven relay box - must read from oficial config software - more on wiki http://RemoteQTH.com/wiki/ip-switch
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
   /* 1  */ {0x55, 0xBA, 0xDD, 0x01, 0xA8, 0xC0, 0xD4, 0x2B, 0xDC, 0x01, 0xA8, 0xC0, 0xD4, 0x2B, 0x01, 0x01, 0xA8, 0xC0, 0x01, 0x80, 0x25, 0x00, 0x03, 0x00, 0x00, 0x04, 0x00, 0xFF, 0xFF, 0xFF, 0x9D }, // 192.168.1.221
   /* 2  */ {0x55, 0xBA, 0xDE, 0x01, 0xA8, 0xC0, 0xD4, 0x2B, 0xDC, 0x01, 0xA8, 0xC0, 0xD4, 0x2B, 0x01, 0x01, 0xA8, 0xC0, 0x01, 0x80, 0x25, 0x00, 0x03, 0x00, 0x00, 0x04, 0x00, 0xFF, 0xFF, 0xFF, 0x9E }, // 192.168.1.222
   /* 3  */ {0x55, 0xBA, 0xDF, 0x01, 0xA8, 0xC0, 0xD4, 0x2B, 0xDC, 0x01, 0xA8, 0xC0, 0xD4, 0x2B, 0x01, 0x01, 0xA8, 0xC0, 0x01, 0x80, 0x25, 0x00, 0x03, 0x00, 0x00, 0x04, 0x00, 0xFF, 0xFF, 0xFF, 0x9F }, // 192.168.1.223
   /* 4  */ {0x55, 0xBA, 0xE0, 0x01, 0xA8, 0xC0, 0xD4, 0x2B, 0xDC, 0x01, 0xA8, 0xC0, 0xD4, 0x2B, 0x01, 0x01, 0xA8, 0xC0, 0x01, 0x80, 0x25, 0x00, 0x03, 0x00, 0x00, 0x04, 0x00, 0xFF, 0xFF, 0xFF, 0xA0 }, // 192.168.1.224
   /* 5  */ {0x55, 0xBA, 0xE1, 0x01, 0xA8, 0xC0, 0xD4, 0x2B, 0xDC, 0x01, 0xA8, 0xC0, 0xD4, 0x2B, 0x01, 0x01, 0xA8, 0xC0, 0x01, 0x80, 0x25, 0x00, 0x03, 0x00, 0x00, 0x04, 0x00, 0xFF, 0xFF, 0xFF, 0xA1 }, // 192.168.1.225
   /* 6  */ {0x55, 0xBA, 0xE2, 0x01, 0xA8, 0xC0, 0xD4, 0x2B, 0xDC, 0x01, 0xA8, 0xC0, 0xD4, 0x2B, 0x01, 0x01, 0xA8, 0xC0, 0x01, 0x80, 0x25, 0x00, 0x03, 0x00, 0x00, 0x04, 0x00, 0xFF, 0xFF, 0xFF, 0xA2 }, // 192.168.1.226
   /* 7  */ {0x55, 0xBA, 0xE3, 0x01, 0xA8, 0xC0, 0xD4, 0x2B, 0xDC, 0x01, 0xA8, 0xC0, 0xD4, 0x2B, 0x01, 0x01, 0xA8, 0xC0, 0x01, 0x80, 0x25, 0x00, 0x03, 0x00, 0x00, 0x04, 0x00, 0xFF, 0xFF, 0xFF, 0xA3 }, // 192.168.1.227
   /* 8  */ {0x55, 0xBA, 0xDD, 0x01, 0xA8, 0xC0, 0xD4, 0x2B, 0xDC, 0x01, 0xA8, 0xC0, 0xD4, 0x2B, 0x01, 0x01, 0xA8, 0xC0, 0x01, 0x80, 0x25, 0x00, 0x03, 0x00, 0x00, 0x04, 0x00, 0xFF, 0xFF, 0xFF, 0x9D }, // 192.168.1.221
   /* 9  */ {0x55, 0xBA, 0xDD, 0x01, 0xA8, 0xC0, 0xD4, 0x2B, 0xDC, 0x01, 0xA8, 0xC0, 0xD4, 0x2B, 0x01, 0x01, 0xA8, 0xC0, 0x01, 0x80, 0x25, 0x00, 0x03, 0x00, 0x00, 0x04, 0x00, 0xFF, 0xFF, 0xFF, 0x9D }, // 192.168.1.221
   /* 10 */ //{0x55, 0xBA, 0xDD, 0x01, 0xA8, 0xC0, 0xD4, 0x2B, 0xDC, 0x01, 0xA8, 0xC0, 0xD4, 0x2B, 0x01, 0x01, 0xA8, 0xC0, 0x01, 0x80, 0x25, 0x00, 0x03, 0x00, 0x00, 0x04, 0x00, 0xFF, 0xFF, 0xFF, 0x9D }, // 192.168.1.221
   /* 10 */ {0x55, 0xBA, 0x46, 0x8E, 0xF9, 0x57, 0xD4, 0x2B, 0xDC, 0x01, 0xA8, 0xC0, 0xD4, 0x2B, 0xC8, 0x01, 0xA8, 0xC0, 0x01, 0x80, 0x25, 0x00, 0x03, 0x00, 0x00, 0x04, 0x00, 0xFF, 0xFF, 0xFF, 0x42 }, // 87.249.142.70:11220 -rc2
   /* 11 */ {0x55, 0xBA, 0xC9, 0x00, 0xA8, 0xC0, 0x2A, 0x20, 0xDD, 0x01, 0xA8, 0xC0, 0xD4, 0x2B, 0x01, 0x01, 0xA8, 0xC0, 0x03, 0x80, 0x25, 0x00, 0x03, 0x00, 0x01, 0x84, 0x00, 0xFF, 0xFF, 0xFF, 0x57 }, // 192.168.1.221 -rel
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
          };
      #endif
#else //------------------------------------------------- relay
      char RXdata[3];
      int nr=1;
      #if defined(controlCFG)
          byte CFGmsgR[4][31] = {  // NOT IMPLEMENTED YET!
            // default ip config 55 BA C9 00 A8 C0 2A 20 DD 01 A8 C0 D4 2B 01 01 A8 C0 03 80 25 00 03 00 01 84 00 FF FF FF 57 

   /* 1  */ {0x55, 0xBA, 0xDD, 0x01, 0xA8, 0xC0, 0xD4, 0x2B, 0xDC, 0x01, 0xA8, 0xC0, 0xD4, 0x2B, 0x01, 0x01, 0xA8, 0xC0, 0x01, 0x80, 0x25, 0x00, 0x03, 0x00, 0x00, 0x04, 0x00, 0xFF, 0xFF, 0xFF, 0x9D }, // 192.168.1.221
   /* 2  */ {0x55, 0xBA, 0xDE, 0x01, 0xA8, 0xC0, 0xD4, 0x2B, 0xDC, 0x01, 0xA8, 0xC0, 0xD4, 0x2B, 0x01, 0x01, 0xA8, 0xC0, 0x01, 0x80, 0x25, 0x00, 0x03, 0x00, 0x00, 0x04, 0x00, 0xFF, 0xFF, 0xFF, 0x9E }, // 192.168.1.222
   /* 3  */ {0x03, 0xBA, 0xDD, 0x01, 0xA8, 0xC0, 0xD4, 0x2B, 0xDC, 0x01, 0xA8, 0xC0, 0xD4, 0x2B, 0x01, 0x01, 0xA8, 0xC0, 0x01, 0x80, 0x25, 0x00, 0x03, 0x00, 0x00, 0x04, 0x00, 0xFF, 0xFF, 0xFF, 0x9D }, //
   /* 4  */ {0x04, 0xBA, 0xDD, 0x01, 0xA8, 0xC0, 0xD4, 0x2B, 0xDC, 0x01, 0xA8, 0xC0, 0xD4, 0x2B, 0x01, 0x01, 0xA8, 0xC0, 0x01, 0x80, 0x25, 0x00, 0x03, 0x00, 0x00, 0x04, 0x00, 0xFF, 0xFF, 0xFF, 0x9D }, //
          };
      #endif
#endif

void setup() {
      Serial.begin(baud);
      #if defined(CONTROL) //----------------------------------- control
            DDRD = DDRD | B00000000; // D7-0 input
            DDRB = B00100000;        // D13-12 input, 13 output
            DDRC = B00100000;        // A0-5 input (A5 output)
            pinMode(CFG,  INPUT);    // A6 input (cfg)
            digitalWrite (13, HIGH); // TCP232 config OFF
      #else //------------------------------------------------- relay
            DDRD = DDRD | B11111100; // D7-0 output
            DDRB = B00111111;        // D8-13 output
            DDRC = B00011111;        // A0-5 output
            pinMode(13, OUTPUT);
            digitalWrite(13, HIGH);  // for TCP232 cfg pin (if used)
      #endif
}

void loop() {
  #if defined(CONTROL) ////////////////////////////////////////////////////////////////// control
        ReadIN();
        timeout = millis()-previous;               // check timeout
        #if defined(fullBits)
            if (cache1 != data1aa || cache1b != data1bb || cache2 != data2 || cache3 != data3){ // if input change or timeout
                if (data1aa > 0 || data1bb > 0 || data2 > 0 || data3 > 0){
                    cache1 = data1aa; cache1b = data1bb; cache2 = data2; cache3 = data3;
                    SendData(0);                     // data withOUT verify bit
                }
                previous = millis();             // set time mark
            }
        #else
            if (cache1 != data1 || cache2 != data2 || cache3 != data3){ // if input change or timeout
                  cache1 = data1; cache2 = data2; cache3 = data3;
                  SendData(0);                     // data withOUT verify bit
                  previous = millis();             // set time mark
            }
        #endif
        
        if (timeout>delayC){
              SendData(0);                     // data withOUT verify bit
              previous = millis();             // set time mark
              PORTC = PORTC & ~ (1<<5);        // A5 LED OFF
        }

        if (Serial.available()) {             // if RX
              ch = toUpperCase(Serial.read());
              if (ch == 'F'){                 // and it is confirm value F
                    PORTC = PORTC | (1<<5);   // A5 LED ON
                    previous = millis();      // set time mark
              }
        }
        #if defined(controlCFG)
              if (analogRead(CFG) < 50) {            // CFG button
              PORTC = PORTC & ~ (1<<5);              // A5 LED OFF
                     delay(3000);
                     if (analogRead(CFG) < 50) {     // ifpush 3 sec
                           ReadIN();
                           digitalWrite (13, LOW);   // TCP232 config ON
                           delay(200);
                                 CFG232();           // write by Control cfg table
                           PORTC = PORTC | (1<<5);   // A5 LED ON
                           delay(3000);
                           digitalWrite (13, HIGH);  // TCP232 config OFF
                     }
              }
        #endif
  #else ////////////////////////////////////////////////////////////////////////////////////////// relay

        while (Serial.available() > 0) {
            #if defined(fullBits)
                data1 = Serial.parseInt();
                data1b = Serial.parseInt();
                data2 = Serial.parseInt();
                data3 = Serial.parseInt();
                if (Serial.read() == '\n') {
                      if ((data2>=0) && (data2<2) && (data3>=0) && (data3<16)){  // if correct range of values (data2 0-4 with verify bit)
                            //if (maskCut(1,data2) == 0){             // if missed verify bit
                            WriteOUT();
                            Serial.print('F');                        // return value
                            previous = millis();                      // set time mark
                      }
                }
            #else
              data1 = Serial.parseInt();
              data2 = Serial.parseInt();
              data3 = Serial.parseInt();
              if (Serial.read() == '\n') {
                    if ((data1>0) && (data1<12) && (data2>=0) && (data2<2) && (data3>=0) && (data3<16)){  // if correct range of values (data2 0-4 with verify bit)
                          //if (maskCut(1,data2) == 0){             // if missed verify bit
                          WriteOUT();
                          Serial.print('F');                        // return value
                          previous = millis();                      // set time mark
                    }
              }
            #endif
        }
        timeout = millis()-previous;                          // check timeout
        if (timeout>(delayR*1000)){
            #if defined(fullBits)
              data1 = 0;
              data1b = 16;
            #else
              data1 = parking;
            #endif
              data2 = 0;
              data3 = 0;
              WriteOUT();
            previous = millis();                      // set time mark
        }
  #endif
}

#if defined(CONTROL) ////////////////////////////////////////////////////////////////// control
      void ReadIN(){
              // http://www.catonmat.net/blog/low-level-bit-hacks-you-absolutely-must-know/
              //------------------------------------------------------------- 1-11 [4 bit] - data1
              int data1a = PIND;
              int data1b = PINB;
              #if defined(fullBits)
                  data1aa = B11111100 & data1a;   // mask bit (read only bit 2-7)
                  data1bb = B00011111 & data1b;   // mask bit (read only bit 0-4)
              #else
                  if (maskCut(2,data1a) == 1){data1 = 1;}            // 1-11 switch to bcd
                        else if (maskCut(3,data1a) == 1){data1 = 2;}
                        else if (maskCut(4,data1a) == 1){data1 = 3;}
                        else if (maskCut(5,data1a) == 1){data1 = 4;}
                        else if (maskCut(6,data1a) == 1){data1 = 5;}
                        else if (maskCut(7,data1a) == 1){data1 = 6;}
                        else if (maskCut(0,data1b) == 1){data1 = 7;}
                        else if (maskCut(1,data1b) == 1){data1 = 8;}
                        else if (maskCut(2,data1b) == 1){data1 = 9;}
                        else if (maskCut(3,data1b) == 1){data1 = 10;}
                        else if (maskCut(4,data1b) == 1){data1 = 11;}
              #endif
              //------------------------------------------------------------- 16 [1 bit] - data2
              int data2a = PINC;
              if (maskCut(4,data2a) == 1){data2 = 1;} // 16 switch to hex
                    else {data2 = 0;}
              //------------------------------------------------------------- 12-15 [4 bit] - data3
              //data3 = data2a & ~ (1<<4);  // set 0 for five bit (sw 16)
              data3 = B00001111 & data2a;   // mask bit (read only bit 0-3)
      }

      int SendData(int verifyBit){
            if (verifyBit == 1){
                  data2b = data2 | (1<<1);  // n-th bit to 1
            }else if(verifyBit == 0){
                  data2b = data2;
            }
            #if defined(fullBits)
                Serial.print(data1aa, DEC);
                Serial.print(',');
                Serial.print(data1bb, DEC);
            #else
                Serial.print(data1, DEC);
            #endif
            Serial.print(',');
            Serial.print(data2b, DEC);
            Serial.print(',');
            Serial.print(data3, DEC);
            Serial.print('\n');
      }
      
#else ////////////////////////////////////////////////////////////////////////////////////////// relay
      void WriteOUT(){
          #if defined(fullBits)
              PORTD = data1;
              PORTB = data1b;
          #else
              //------------------------------------------------------------- 1-11 [4 bit] - data1
              if (data1 == 1){PORTD = B00000100; PORTB = B00100000;}          // 1-11 hex to switch
                    else if (data1 == 2){PORTD = B00001000; PORTB = B00100000;}             // B xx1xxxxx for TCP232 cfg pin
                    else if (data1 == 3){PORTD = B00010000; PORTB = B00100000;}
                    else if (data1 == 4){PORTD = B00100000; PORTB = B00100000;}
                    else if (data1 == 5){PORTD = B01000000; PORTB = B00100000;}
                    else if (data1 == 6){PORTD = B10000000; PORTB = B00100000;}
                    else if (data1 == 7){PORTD = B00000000; PORTB = B00100001;}
                    else if (data1 == 8){PORTD = B00000000; PORTB = B00100010;}
                    else if (data1 == 9){PORTD = B00000000; PORTB = B00100100;}
                    else if (data1 == 10){PORTD = B00000000; PORTB = B00101000;}
                    else if (data1 == 11){PORTD = B00000000; PORTB = B00110000;}
          #endif                    
              //------------------------------------------------------------- 16 [1 bit] - data2
              if (maskCut(0,data2) == 1){   // if bit0 =1 
                      data3 = data3 | (1<<4);
              }
              //------------------------------------------------------------- 12-15 [4 bit] - data3
              PORTC = data3;
      }
#endif

int maskCut(int bitNR, int data){    // Test if the n-th bit is set
      if (data & (1<<bitNR)) {
        return 1;
      }
      else {
        return 0;
      }
}

#if defined(controlCFG)
    int CFG232(){
          if (baud != 9600) {Serial.begin(9600);}
          for (int x=0; x<31; x++){ 
            #if defined(CONTROL)
                Serial.write(CFGmsgC[data1-1][x]);          // write config value from 'control table'
            #else
                Serial.print(CFGmsgR[???????][x], HEX);     // write config value from 'relay table' - NOT implemeted yet! (choose cfg line)
            #endif
            //    Serial.print('|');
          } 
          //Serial.println(' ');
          
          if (baud != 9600) {Serial.begin(baud);}
    }
#endif

/*==================================================================================== checksum for future convert preset to hex code

void setup()
{
   Serial.begin(9600);
}


// 0x55, 0xBA, 0xDD, 0x01, 0xA8, 0xC0, 0xD4, 0x2B, 0xDC, 0x01, 0xA8, 0xC0, 0xD4, 0x2B, 0x01, 0x01, 0xA8, 0xC0, 0x01, 0x80, 0x25, 0x00, 0x03, 0x00, 0x00, 0x04, 0x00, 0xFF, 0xFF, 0xFF, 0x9D }, // 192.168.1.221
// 0x55, 0xBA, 0xDE, 0x01, 0xA8, 0xC0, 0xD4, 0x2B, 0xDC, 0x01, 0xA8, 0xC0, 0xD4, 0x2B, 0x01, 0x01, 0xA8, 0xC0, 0x01, 0x80, 0x25, 0x00, 0x03, 0x00, 0x00, 0x04, 0x00, 0xFF, 0xFF, 0xFF, 0x9E }, // 192.168.1.222

//byte pack[28] = {0xDD, 0x01, 0xA8, 0xC0, 0xD4, 0x2B, 0xDC, 0x01, 0xA8, 0xC0, 0xD4, 0x2B, 0x01, 0x01, 0xA8, 0xC0, 0x01, 0x80, 0x25, 0x00, 0x03, 0x00, 0x00, 0x04, 0x00, 0xFF, 0xFF, 0xFF};
byte pack[28] = {0xDE, 0x01, 0xA8, 0xC0, 0xD4, 0x2B, 0xDC, 0x01, 0xA8, 0xC0, 0xD4, 0x2B, 0x01, 0x01, 0xA8, 0xC0, 0x01, 0x80, 0x25, 0x00, 0x03, 0x00, 0x00, 0x04, 0x00, 0xFF, 0xFF, 0xFF};
int pack_size = 28;

void loop()
{
   byte sum = 0;
   for (int i = 0; i < pack_size-1; i++) {
       sum += pack[i];
   }
   byte crcbyte = pack[pack_size-1];
   byte overall = sum+crcbyte;
   printhexbyte(overall);
   delay(2000);
}

void printhexbyte(byte x)
{
   Serial.print("0x");
   if (x < 16) {
       Serial.print('0');
   }
   Serial.print(x, HEX);
   Serial.println();
}

*/
