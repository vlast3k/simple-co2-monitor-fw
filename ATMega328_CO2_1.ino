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
#define ONE_HOUR 60L*1000

//#define DEF_DEBUG
#define ANALOG_READ_PRECISION 30

#define DEF_DEBUG true
boolean DEBUG=DEF_DEBUG;
//#define ANALOG_READ_PRECISION 15
//uuuuu
//#else
//boolean DEBUG=false;
////kjkjkj
////#define ANALOG_READ_PRECISION 30
//#endif

//rrrr
#define EE_FLT_CURRENT_PERIOD_CO2_HIGHESTMV  0
#define EE_FLT_PREV_PERIOD_CO2_HIGHESTMV 4
#define EE_4B_HOUR 8
#define EE_B_HITEMP 12
#define EE_B_LOTEMP 13
#define EE_10B_TH 20

void read1sec();
void processBrightness(byte);
double analogReadFine(int, byte); 
void storeColorRanges(char *);
int espCheckBaudRate();
void startSerialProxy();


double currentCO2MaxMv = 0;
double prevCO2MaxMv = 0;
RunningAverage raCO2mv(4);
RunningAverage raCO2mvNoTempCorr(4);
RunningAverage raTempC(4);
RunningAverage raLight(4);

SoftwareSerial esp(ESP_RX, ESP_TX); // RX, TX
boolean hasESP;
boolean overrideLeds = false;
byte overrideBrightness = 255;

void setup() {
  Serial.begin(9600);
  Serial.println(F("ATMega is Alive"));
  hasESP = false;//espFixBaudRate();
  espOFF();

  initCO2ABC();

  int tt;
  Serial << "ee" << EEPROM.read(100) << endl;

  initNeopixels();
}


byte sBrightness = 10;
uint16_t sPPM = 0;

void displayDebugInfo() {
  debugInfoCO2ABC();
  debugInfoNeopixel();
  Serial << endl;
}

void loop() {
  processCO2();
  oledTechnicalDetails();
  //oledAll();
  processNeopixels();
  displayDebugInfo();
  processUserInput();
//  delay(1000);
//
//  sendToThingSpeak("", 234);
//  fixBaudRate();
//  setESPWifiPass("", "");
//  serialProxy();
  
}


    



