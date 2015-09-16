#define LINE_LEN 30
char line[LINE_LEN];
byte readLine(int timeout) {
  unsigned long deadline = millis() + timeout;
  byte i = 0;
  while (millis() < deadline) {
    if (Serial.available()) {
      line[i++] = (char) Serial.read();
      if (i == LINE_LEN - 1) {
        break;
      } else if (line[i-1] == 10 || line[i-1] == 13) {
        line[i-1] = 0;
        Serial.flush();
        return i-1;
      }
    }
  }
  line[i] = 0;
  return i;
}

typedef int (*MenuCallback) ();

MenuCallback menuHandler;

int menuPrintMain() {
  Serial << endl << F("(1) Set current CO2 Level") << endl;
  Serial << F("(2) Set LED Brightness") << endl;
  Serial << F("(3) Set LED Tesholds") << endl;
  if (hasESP) Serial << F("(4) Configure WIFI") << endl;
  Serial << F("(r) Reset Configuration (no prompt!)") << endl;
  #ifdef DEF_DEBUG
  Serial << F("(n) Max Light") << endl;
  Serial << F("(m) Min Light") << endl;
  #endif
  Serial << F("Select Option or (q)uit: ");
  menuHandler = menuChooseOption;
    return 1;

}

int menuWifi() {
  Serial << endl << F("(1) Set SSID") << endl;
  Serial << F("(2) Set Password") << endl;
  Serial << F("(3) Connect") << endl;
  Serial << F("(4) Start proxy to ESP8266") << endl;
  Serial << F("Select Option or (q)uit: ");
  menuHandler = handleWifi;
    return 1;

}

String ssid = "vladiHome";
String pass = "0888414447";

int wifiEnterSSID() {
  ssid = String(line);
  Serial << F("SSID: ") << ssid  << endl;
  menuWifi();
  return 1;
}

int wifiEnterPass() {
  pass = String(line);
  Serial << F("PASS: ") << pass  << endl;
  menuWifi();
  return 1;
}

int handleWifi() {
  switch (line[0]) {
    case '1':
      Serial << endl << F("Enter SSID: ") << endl;
      menuHandler = wifiEnterSSID;
      break;
    case '2':
      Serial << endl << F("Enter Password: ") << endl;
      menuHandler = wifiEnterPass;
      break;
    case '3':
      Serial << endl << F("Connecting to Wifi...") << endl;
      espToggle();
      if (!serialFind("ready", DEBUG, 6000)) {
        Serial << F("Wifi module not working") << endl;
      } else {
        Serial.flush();
        delay(1000);
        int res = setESPWifiPass(ssid.c_str(), pass.c_str());
//        int res = setESPWifiPass("vladiHome", "0888414447");
        if (res < 0) {
          Serial << F("Could not connect to Wifi");
        }
      }
      menuWifi();
      break;
    case '4':
      startSerialProxy();
      break;
    default:
      menuPrintMain();
  }
  return 1;
}

int menuChooseOption() {
  int *rng;
  switch (line[0]) {
    case '1':
      Serial << endl << F("Enter value in ppm or 'q' to go back");
      menuHandler = menuEnterCO2Val;
      break;
    case '2':
      Serial << endl << F("Enter brightness 0 - 255 or 'q' to go back") << endl;
      menuHandler = menuEnterBrightVal;
      break;
    case '3':
      Serial << endl << F("Current ranges: ");
      int r;
      for (byte i=0; i < 5; i++)  {
        EEPROM.get(EE_10B_TH + i*2, r);
        Serial << r << F(" ");
      }
      Serial << endl << F("Enter color ranges, e.g: 700 1000 1300 1600 1900, or 'd' for defaults") << endl;
      menuHandler = menuEnterColorRanges;
      break;
    case '4':
      if (hasESP) menuWifi();
      break;
    case 'r':
      for (int i=0; i < 100; i++) EEPROM.write(i, 255);
      Serial << F("Configuration reset. Please restart device") << endl;
      return 0;
    case 'm':
      overrideLeds = !overrideLeds;
      Serial << F("Override Leds:") << overrideLeds << endl;
      return 0;
    case 'q':
      Serial << endl << F("Leaving menu") << endl;
      return 0;
  }
  return 1;
}

int menuEnterCO2Val() {
  Serial <<"entercoval" << endl;
  delay(200);
  sPPM = String(line).toInt();
  if (sPPM) processNeopixels();
  menuHandler = menuPrintMain;
  menuPrintMain();
  return 1;
}

int menuEnterBrightVal() {
  sBrightness = (byte)String(line).toInt();
  if (sBrightness) overrideBrightness = sBrightness;
//  /menuHandler = menuPrintMain;
  //menuPrintMain();
  Serial << F("Done") << endl;
  return 0;
}

int menuEnterColorRanges() {
  storeColorRanges(line);
  processNeopixels();
  menuHandler = menuPrintMain;
  menuPrintMain();
  return 1;
}

void dumpLine(char* str) {
  Serial << "linedump: " ;
  for (int i=0; i<20; i++) {
    Serial << (byte) str[i] << ",";
  }
  Serial <<endl;
}

void removeCRNL(char * str) {
  for (int i=0; i < strlen(str); i++) {
    if (str[i] == 10 || str[i] == 13) { 
      str[i] =0;
      return;
    }
  }
}

void processUserInput() {
  byte len = 0;
  menuHandler = menuPrintMain;

  Serial.setTimeout(1000);
  if (!Serial.available()) return;
  
  Serial.setTimeout(30000);
  while (readLine(30000) >= 0) {
    Serial.flush();
    removeCRNL(line);
    if (!menuHandler()) return;
  }

  Serial << "Menu Timed out" << endl;
}
      
