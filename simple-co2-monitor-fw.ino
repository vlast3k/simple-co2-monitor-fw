#include <RunningAverage.h>
#include <SoftwareSerial.h>
#include <avr/power.h>
#include <Streaming.h>
#include <EEPROM.h>

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
//#define ANALOG_READ_PRECISION 15
//uuuuu
//#else
//boolean DEBUG=false;
////kjkjkj
////#define ANALOG_READ_PRECISION 30
//#endif

//rrrr
#define EE_FLT_CURRENT_PERIOD_CO2_HIGHESTMV  4
#define EE_FLT_PREV_PERIOD_CO2_HIGHESTMV 8
#define EE_4B_HOUR 12
#define EE_10B_TH 16
#define EE_1B_WIFIINIT 26
#define EE_30B_TSKEY 27
#define EE_1B_HASWIFI 57
#define EE_1B_BRG 58
#define EE_1B_ISGRAY 59

#define EE_VERSION 3

void read1sec();
void processBrightness(byte);
double analogReadFine(int, byte); 
void storeColorRanges(char *);
int espCheckBaudRate();
int startSerialProxy();
void softwareReset();
int sendToThingSpeak(int);


double currentCO2MaxMv = 0;
double prevCO2MaxMv = 0;
RunningAverage raCO2mv(4);
RunningAverage raCO2mvNoTempCorr(4);
RunningAverage raTempC(4);
RunningAverage raLight(4);
boolean startedCO2Monitoring = false;

SoftwareSerial esp(ESP_RX, ESP_TX); // RX, TX
boolean hasESP;
//boolean overrideLeds = false;
byte overrideBrightness = EEPROM.read(EE_1B_BRG);
boolean dumpDebuggingInfo = false;
byte sBrightness = 10;
uint16_t sPPM = 0;
char *wifiStat = "n/a";
void setup() {
  
  Serial.begin(9600);
  if (DEBUG) {
    Serial << endl << F(".................DEBUG IS ON") << endl <<endl;
  } 
  Serial << F("Starting... v1.2.1") << endl;
  Serial << F("Box color is: ") << EEPROM.read(EE_1B_ISGRAY) << endl;
  checkEEVersion();
  initNeopixels();
  if (EEPROM.read(EE_1B_HASWIFI) == 2) {
    hasESP = false;
  } else {
    hasESP = espFixBaudRate();
    EEPROM.update(EE_1B_HASWIFI, hasESP? 1:2);
  }
  if (hasESP) Serial << F("WiFi Enabled") << endl;
  else Serial << F("No WiFi installed") << endl;
  espOFF();
  initCO2ABC();
  if (hasESP) setWifiStat("n/a");
  else setWifiStat("");
  //initCO2ABC();
//  sPPM= 2222;
  Serial.println(F("Simple CO2 Monitor. Press any key to display menu"));
}

void checkEEVersion() {
  byte eeVersion = EEPROM.get(0, eeVersion);
  Serial << F("EEPROM Version: ") << eeVersion << endl;
  if (eeVersion != EE_VERSION) {
    clearEEPROM();
    EEPROM.put(0, EE_VERSION);
  }
}

void setWifiStat(char* st) {
  if (!hasESP) return;
  wifiStat = st;
  oledCO2Level();
}

void displayDebugInfo() {
  debugInfoCO2ABC();
  debugInfoNeopixel();
  Serial << endl;
}

void loop() {
  processCO2();
  //oledTechnicalDetails();
  //oledAll();
  processNeopixels();
  if (dumpDebuggingInfo) {
    displayDebugInfo();
    oledTechnicalDetails();
  } else {
    oledCO2Level();
  }
  processUserInput();
  processSendData();
//  delay(1000);
//
//  sendToThingSpeak("", 234);
//  fixBaudRate();
//  setESPWifiPass("", "");
//  serialProxy();
  
}


    



