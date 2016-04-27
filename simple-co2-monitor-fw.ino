//#define TGS4161
//#define GRAY
#define BRG

#ifndef TGS4161
#include <CubicGasSensors.h>
#endif

#ifndef USELIB
#endif

#include <RunningAverage.h>
#include <SoftwareSerial.h>
//#include <avr/power.h>
#include <Streaming.h>
#include <EEPROM.h>
//#include <MemoryFree.h>
//#include <Timer.h>

//#define PIN            A0
#define PIN            A1

#define ESP_RX 9
#define ESP_TX 10
#define ESP_CHPD 2

#define LIGHT_PIN A0
#define TEMP_PIN A3
#define CO2_PIN A2
//#define LIGHT_PIN A1
//#define TEMP_PIN A2
//#define CO2_PIN A3
#define VIN 5.15d

#define TIMEOUT_READ_SENSORS 5000
#define TIMEOUT_LIGHT 10000

//#define DEBUG
#define ANALOG_READ_PRECISION 10

boolean DEBUG=false;
boolean ESP_DEBUG = true;


//#define ANALOG_READ_PRECISION 15
//uuuuu
//#else
//boolean DEBUG=false;
////kjkjkj
////#define ANALOG_READ_PRECISION 30
//#endif

//rrrr

#define EE_10B_TH 16
#define EE_1B_WIFIINIT 26

//#define EE_1B_HASWIFI 57
#define EE_1B_BRG 58
#define EE_1B_ISGRAY 59
#define EE_40B_UBIKEY 60
#define EE_40B_UBIVAR 100
#define EE_40B_TSKEY  140
#define EE_1B_HASSAPCFG    180
#define EE_2B_WIFI_SND_INT_S  181
#define EE_1B_BRG_FACTOR  183
#define EE_1B_RESET_CO2  184

#define EE_VERSION 3

void processBrightness(byte);
double analogReadFine(int, byte); 
void storeColorRanges(char *);
int espCheckBaudRate();
int startSerialProxy();
void softwareReset();
int sendToThingSpeak(int);
#ifndef TGS4161
void debugInfoCM1106();
#endif

boolean startedCO2Monitoring = false;

#ifdef TGS4161

#define EE_FLT_CURRENT_PERIOD_CO2_HIGHESTMV  4
#define EE_FLT_PREV_PERIOD_CO2_HIGHESTMV 8
#define EE_4B_HOUR 12
double currentCO2MaxMv = 0;
double prevCO2MaxMv = 0;
RunningAverage raCO2mv(4);
RunningAverage raCO2mvNoTempCorr(4);
RunningAverage raTempC(4);
#else
//TX on CO2 Sensor to CO2 pad
#define SS_RX CO2_PIN

//RX on CO2 Sensor to TMP pad
#define SS_TX TEMP_PIN
#endif

RunningAverage raLight(4);
RunningAverage raCO2Change(4);

SoftwareSerial esp(ESP_RX, ESP_TX); // RX, TX
//boolean overrideLeds = false;
byte overrideBrightness;
//float brgFactor;
boolean dumpDebuggingInfo = false;
byte sBrightness = 10;
uint16_t sPPM = 0;
char *wifiStat = "n/a";

#ifndef TGS4161
 CubicGasSensors cubicCo2(SS_RX, SS_TX, EE_1B_RESET_CO2);
#endif

//Timer *beepTimer = new Timer(60L*5L*1000L);
/*
 * MAX Sketch size should be less than 0x7000 28672 bytes to work with stupid bootloaders
 */
void setup() {
  Serial.begin(9600);  
 // esp.begin(9600);
  if (DEBUG) {
    Serial <<  F("\n\nDeG\n\n");
  } 
#ifdef TGS4161
  Serial << F("vAir CO2 Monitor: v1.12\n");// << endl;
#else
  Serial << F("vAir CO2 Monitor NDIR: v1.12\n");// << endl;
#endif
  Serial << F("Visit 'vair-monitor.com' for configuration details\n");// << endl;
  int16_t wifiSendInterval;
  EEPROM.get(EE_2B_WIFI_SND_INT_S, wifiSendInterval);
  Serial << F("Send Interval: ") << wifiSendInterval << endl;
  if (!isWifiInit()) Serial << F("No WiFi configuration\n");
//  Serial << CM1106__getCO2() << endl;
//  startSerialP  roxy();
//  #ifndef TGS4161
//  Serial << CM1106__getCO2() << endl;
//  Serial << CM1106__getCO2() << endl;
//  Serial << CM1106__getCO2() << endl;
//  #endif
  overrideBrightness = EEPROM.read(EE_1B_BRG);
  //checkEEVersion();
  initNeopixels();
  
  espOFF();
#ifdef TGS4161
  initCO2ABC();
//#else
//  Serial << F("CO2 value: ") << rawReadCM1106_CO2() << endl;
#endif
  setWifiStat("");
  //makeBeep();
 // tone(A4, 200);

  
 //beepTimer->setOnTimer(&handleBeep);
 // beepTimer->Start();
  //tone(1, 20000, 1000);
  //initCO2ABC();
//  sPPM= 2222;
//  Serial.println(F("Simple CO2 Monitor. Press any key to display menu"));
}

//void checkEEVersion() {
//  byte eeVersion = EEPROM.get(0, eeVersion);
//  //Serial << F("EEPROM Version: ") << eeVersion << endl;
//  if (eeVersion != EE_VERSION) {
//    clearEEPROM();
//    EEPROM.put(0, EE_VERSION);
//  }
//}

void setWifiStat(char* st) {
  if (!isWifiInit()) return;
  wifiStat = st;
  oledCO2Level();
}

void displayDebugInfo() {
  debugInfoCO2ABC();
  #ifndef TGS4161
    cubicCo2.printDebugInfo();
  #endif
  debugInfoNeopixel();
  Serial << endl;
}

//void handleBeep() {
// // Serial << F("testBEEP\n");
//  if (sPPM > 1800 && sBrightness > 1) makeBeep();
//}

void loop() {
  //Serial <<"." << endl;
#ifdef TGS4161
  processCO2();
#else
  sPPM = cubicCo2.getCO2();
  startedCO2Monitoring = cubicCo2.hasStarted();
  delay(3000);
#endif
  //oledTechnicalDetails();
  //oledAll();
  processNeopixels();
  if (dumpDebuggingInfo) {
    displayDebugInfo();
    //oledTechnicalDetails();
  } 
  //else {
    oledCO2Level();
 // }
  processUserInput();
  processSendData();
  //beepTimer->Update();
  //delay(10000);
//
//  sendToThingSpeak("", 234);
//  fixBaudRate();
//  setESPWifiPass("", "");
//  serialProxy();
  
}


    



