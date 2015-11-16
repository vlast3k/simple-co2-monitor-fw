#define LINE_LEN 30
char line[LINE_LEN];
typedef int (*MenuCallback) ();
MenuCallback menuHandler;
String ssid;
String pass;

void processUserInput() {
  byte len = 0;
  menuHandler = menuPrintMain;

  Serial.setTimeout(1000);
  if (!Serial.available()) return;
  
  Serial.setTimeout(30000);
  while (readLine(30000) >= 0) {
    Serial.flush();
    removeCRNL(line);
    if (strlen(line) > 2) {
      if (!handleCommand()) {
        Serial << endl << F("Exiting menu") << endl;
        return;
      }
    }
    else if (!menuHandler()) {
      Serial << endl << F("Exiting menu") << endl;
      return;
    }
  }

  Serial << F("Menu Timed out") << endl;
}

int handleCommand() {
  String data = String(line).substring(2);
  if (line[0] == 's') ssid = data;
  if (line[0] == 'p') pass = data;
  if (line[0] == 'c') doConnect();
  if (line[0] == 'b') {
    overrideBrightness = data.toInt();
    processBrightness();   
  }
  if (line[0] == 'o') {
    sPPM = data.toInt();
    processColors();
    oledCO2Level();
  }
  if (line[0] == 't') {
    doSetTSKey(data.c_str());
  }
  
  if (line[0] == 'd') {
    switchDebugInfoPrint();
    return 0;
  }

  Serial << F("OK") << endl;
  //if (line[0]
  return 1;
}

byte readLine(int timeout) {
  unsigned long deadline = millis() + timeout;
  byte i = 0;
  while (millis() < deadline) {
    if (Serial.available()) {
      line[i++] = (char) Serial.read();
       if (line[i-1] == 10) {
          i--;
          continue;
        }

      if (i == LINE_LEN - 1) {
        break;
      } else if (line[i-1] == 13) {
        line[i-1] = 0;
        Serial.flush();
        return i-1;
      }
    }
  }
  line[i] = 0;
  return i;
}

void removeCRNL(char * str) {
  for (int i=0; i < strlen(str); i++) {
    if (str[i] == 10 || str[i] == 13) { 
      str[i] =0;
      return;
    }
  }
}

// --------- MAIN menu handlers -----------------------

int menuPrintMain() {
  Serial << endl << F("(1) Set LED Thresholds") << endl;
  if (hasESP) Serial << F("(w) Configure WIFI") << endl;
 // Serial << F("(b) Set LED Brightness") << endl;
  Serial << F("(d) Enable display debugging info") << endl;
  Serial << F("(r) Factory Reset (no prompt!)") << endl;
  if (DEBUG) Serial << F("(s) Simulate CO2") << endl;
  Serial << F("Select Option or (q)uit: ");
  menuHandler = menuChooseOption;
  return 1;
}

int menuChooseOption() {
  if (line[0] == '1')           return menuMainDisplayColorRanges();
  if (line[0] == 'w' && hasESP) return menuWifi(); 
  if (line[0] == 'b')           return menuSetLedBrightness(); 
  if (line[0] == 'r')           return menuMainFactoryReset(); 
  if (line[0] == 'd')           return switchDebugInfoPrint(); 
  if (line[0] == 'g')           return switchGrayBox();
  if (line[0] == 's' && DEBUG) return simulateCO2();
  return 0;
}  

int menuSetLedBrightness() {
  Serial << endl << F("Current value: ") << overrideBrightness<< endl;
  Serial << F("Enter LED Brightness 0-100% (255 for auto)") << endl;
  menuHandler = menuEnterLedBrightness;
  return 1;
}

int menuEnterLedBrightness() {
  //Serial  << line << ",," << String(line).toInt() << endl;
  int res = String(line).toInt();
  if (res < 255) res = constrain(res, 0, 150);
  overrideBrightness = res;
  Serial << F("New Brightness value: ") << overrideBrightness << endl;
  EEPROM.write(EE_1B_BRG, overrideBrightness);
  //menuHandler = menuPrintMain;
  return 0;
}

int menuMainDisplayColorRanges() {
  Serial << endl << F("Current ranges: ");
  int r;
  for (byte i=0; i < 5; i++)  {
    EEPROM.get(EE_10B_TH + i*2, r);
    Serial << r << F(" ");
  }
  Serial << endl << F("Enter color ranges, e.g: 700 1000 1300 1600 1900, or 'd' for defaults") << endl;
  menuHandler = menuEnterColorRanges;
  return 1;
}

int menuEnterColorRanges() {
  storeColorRanges(line);
  processNeopixels();
  Serial << F("Done") << endl;
  return 0;
}

int menuMainFactoryReset() {
  clearEEPROM();
  Serial << endl << F("Configuration reset...") << endl;
  softwareReset();
  return 0;
}

int switchGrayBox() {
  byte val = EEPROM.read(EE_1B_ISGRAY);
  if (val == 255) val = 1;
  else val = 255;
  EEPROM.update(EE_1B_ISGRAY, val);
  Serial << endl << F("Box color set to: ");
  if (val == 1) Serial <<("GRAY");
  else Serial << F("WHITE");
  Serial << endl;
  return 0;
}

int switchDebugInfoPrint() {
  dumpDebuggingInfo = !dumpDebuggingInfo;
  Serial << endl  << F("Debug Info is: ") << dumpDebuggingInfo << endl;
  if (dumpDebuggingInfo) displayDebugInfo();
  return 0;
}

int simulateCO2() {
  for (int i = 400; i <=3000; i+=100) {
      sPPM = i;
      processNeopixels();
      oledCO2Level();
      delay(250);
  }
  return menuPrintMain();
}

// --------------- WIFI Menu ------------------------

int menuWifi() {
  Serial << endl << F("(1) Set SSID") << endl;
  Serial << F("(2) Set Password") << endl;
  Serial << F("(3) Connect") << endl;
//  Serial << F("(4) Start proxy to ESP8266") << endl;
  Serial << F("(4) Set ThingSpeak API Key") << endl;
  Serial << F("Select Option or (q)uit: ");
  menuHandler = handleWifi;
  return 1;
}

int handleWifi() {
  if (line[0] == '1') return menuWifiEnterSSID();
  if (line[0] == '2') return menuWifiEnterPass();
  if (line[0] == '3') return menuWifiConnect();
  if (line[0] == 'p') return startSerialProxy();
  if (line[0] == '4') return menuWifiEnterTSKey();
  return menuPrintMain();
}

int menuWifiEnterSSID() {
  Serial << endl << F("Enter SSID: ") << endl;
  menuHandler = onWifiEnterSSID;
  return 1;
}

int onWifiEnterSSID() {
  ssid = String(line);
  Serial << F("SSID: ") << ssid  << endl;
  menuWifi();
  return 1;
}

int menuWifiEnterPass() {
  Serial << endl << F("Enter Password: ") << endl;
  menuHandler = onWifiEnterPass;
  return 1;
}

int onWifiEnterPass() {
  pass = String(line);
  Serial << F("PASS: ") << pass  << endl;
  menuWifi();
  return 1;
}

int doConnect() {
  Serial << endl << F("Connecting to Wifi...") << endl;
  espToggle();
  if (!serialFind("ready", DEBUG, 6000)) {
    Serial << F("Wifi module not working") << endl;
  } else {
    Serial.flush();
    delay(1000);
    int res = setESPWifiPass(ssid.c_str(), pass.c_str());
    if (res < 0) {
      Serial << F("Could not connect to Wifi");
    } else {
      setWifiStat("WiFi OK");
    }
  }
}

int menuWifiConnect() {
  doConnect();
  return menuWifi();  
}

int menuWifiEnterTSKey() {
  EEPROM.get(EE_30B_TSKEY, line);
  Serial << endl << F("Enter ThingSpeak Key: ") << line << endl;
  menuHandler = onWifiEnterTSKey; 
  return 1;
}

void doSetTSKey(const char *key) {
  char kk3[30];
  strcpy(kk3, key);
  EEPROM.put(EE_30B_TSKEY, kk3);
  Serial << F("Testing connection by sending 456 ppm value") << endl;
  sendToThingSpeak(456);
  Serial << F("Done. Please check if 456 was received.") << endl;  
}
int onWifiEnterTSKey() {
  doSetTSKey(line);
  return 0  ;
}

      
