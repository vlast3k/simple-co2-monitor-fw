//#define TGS4161
//#define GRAY
#define BRG

#include <RunningAverage.h>
#include <SoftwareSerial.h>
//#include <avr/power.h>
#include <Streaming.h>
#include <EEPROM.h>
//#include <MemoryFree.h>
//#include <Timer.h>
#include <Adafruit_NeoPixel.h>

boolean timePassed(unsigned long since, unsigned long interval);

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
//boolean ESP_DEBUG = true;
bool hasESP = false;
bool espWifiConfigured = false; 
bool espWifiConnected = false; 
bool espSentInit = false;

uint32_t espLastActivity = 0;
#define LINE_LEN 100
char line[LINE_LEN];
enum CommandSource {FROM_SERIAL, FROM_ESP};
bool espIsOn = false;
bool espWasTurnedOff = false;
bool espStoppedOnce = false;

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
#define EE_1B_GRADIENT  185
#define EE_FLT_PREV_PERIOD_CO2_ATTEMP 186

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

void onLED(String &x);
boolean startedCO2Monitoring = false;

#ifdef TGS4161

#define EE_FLT_CURRENT_PERIOD_CO2_HIGHESTMV  4
#define EE_FLT_PREV_PERIOD_CO2_HIGHESTMV 8
#define EE_4B_HOUR 12
double currentCO2MaxMv = 0;
double prevCO2MaxMv = 0;
double currentMaxMvTemp = 0;
double prevMaxMvTemp = 0;
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
int32_t sPPM = 0;
char *wifiStat = "n/a";

#define NUMPIXELS      6
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

#ifndef TGS4161
//#include "CubicGasSensors\CubicGasSensors.h"
#include <CubicGasSensors.h>
void onCo2Status(CubicStatus status) {
} 

CubicGasSensors cubicCo2(onCo2Status, EE_1B_RESET_CO2, &Serial, SS_RX, SS_TX);
//CubicGasSensors cubicCo2(onCo2Status, EE_1B_RESET_CO2, &Serial, 1,1);
#endif

//Timer *beepTimer = new Timer(60L*5L*1000L);
/*
 * MAX Sketch size should be less than 0x7000 28672 bytes to work with stupid bootloaders
 */
#define LOG_OF_350 log10(400.0d)
uint8_t GRADIENT = 71;
double mv2ppm(double mv, int grad) { return pow((double)10, mv / grad + LOG_OF_350); }
double ppm2mv(int ppm, int grad) { return (log10((double)ppm) - LOG_OF_350) * grad; }
void findGradient();
void testGradient() ;
void setup() {
  //startSerialProxy();

  Serial.begin(9600);  
  String rr = "LED##12 13 14";
  strcpy(line, rr.c_str());
  onLED(rr);
//  Serial << "ra dev" << endl;
//  raDeviation(raTempC);
//  Serial << "end ra dev" << endl;
  EEPROM.get(EE_1B_GRADIENT, GRADIENT);
  if (GRADIENT == 255) GRADIENT = 71;
  Serial << "Gradient: " << GRADIENT << endl;
//  for (int i=400; i < 2000; i+=200) Serial << "ppm:" << i << ",mv: " << ppm2mv(i, GRADIENT) << endl;
  //findGradient();
  //delay(10000);
 // esp.begin(9600);
  if (DEBUG) {
    Serial <<  F("\n\nDeG\n\n");
  } 
#ifdef TGS4161
  Serial << F("vAir CO2 Monitor: v2.0\n");// << endl;
#else
  Serial << F("vAir CO2 Monitor NDIR: v2.0\n");// << endl;
#endif
  Serial << F("Visit 'vair-monitor.com' for configuration details\n");// << endl;
  int16_t wifiSendInterval;
  EEPROM.get(EE_2B_WIFI_SND_INT_S, wifiSendInterval);
  Serial << F("Send Interval: ") << wifiSendInterval << endl;
  //if (!isWifiInit()) Serial << F("No WiFi configuration\n");
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
#ifdef TGS4161
  initCO2ABC();
#else
  //cubicCo2.init();
  //Serial << F("CO2 value: ") << cubicCo2.rawReadCM1106_CO2() << endl;
#endif
  setWifiStat("");

  loopTGS4161();
  //espOFF();
  espToggle();
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
  if (!hasESP) return;
  wifiStat = st;
  oledCO2Level();
}
uint32_t lastDebugInfoPrint = 0;
void displayDebugInfo() {
  if (!timePassed(lastDebugInfoPrint, 5000)) return;
  lastDebugInfoPrint = millis();
  debugInfoCO2ABC();
  #ifndef TGS4161
    //cubicCo2.printDebugInfo();
  #endif
  debugInfoNeopixel();
  Serial << endl;
}

//void handleBeep() {
// // Serial << F("testBEEP\n");
//  if (sPPM > 1800 && sBrightness > 1) makeBeep();
//}
void loopTGS4161() {
#ifdef TGS4161
  if (!espIsOn || timePassed(espLastActivity, 15000)) {
    //Serial << "process CO2 calling"<< endl;
    processCO2();
  }
#endif
}

void loopCubic() {
#ifndef TGS4161
  if (!espIsOn || timePassed(espLastActivity, 15000)) {
    //Serial << "time passed: " << timePassed(espLastActivity, 20000) << ", " << millis() - espLastActivity << endl;
    espPause();
    int x = cubicCo2.getCO2(DEBUG);
    espResume();
    startedCO2Monitoring = cubicCo2.hasStarted();
    if (startedCO2Monitoring) sPPM = x;
    if (x == -1) sPPM = -1;
    delay(1000);
  }
#endif
}

void loop() {
  //Serial <<"." << endl;
//  if (espIsOn && !espStoppedOnce && (millis() > 5L*60*1000)) {
//    espOFF();
//    espStoppedOnce = true;
//  }
  loopTGS4161();
  loopCubic();
  //int x = cubicCo2.getCO2(DEBUG);
//  startedCO2Monitoring = cubicCo2.hasStarted();
//  if (startedCO2Monitoring) sPPM = x;
//  if (x == -1) sPPM = -1;
  //delay(3000);
//#endif
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
  //Serial << "." << endl;
  //beepTimer->Update();
  //delay(10000);
  //delay(100);
//
//  sendToThingSpeak("", 234);
//  fixBaudRate();
//  setESPWifiPass("", "");
//  serialProxy();
  
}


    



