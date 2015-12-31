#define LINE_LEN 60
char line[LINE_LEN];

void processUserInput() {
  if (!Serial.available()) return;
  if (readLine(30000) >= 0) handleCommand();
}

byte readLine(int timeout) {
  unsigned long deadline = millis() + timeout;
  byte i = 0;
  while (millis() < deadline) {
    if (Serial.available()) {
      line[i++] = (char) Serial.read();
      if      (line[i-1] == '\r')  i--;   
      else if (i == LINE_LEN - 1)  break; 
      else if (line[i-1] == '\n')  {i--; break;}
    }
  }
  line[i] = 0;
  return i;
}


void handleCommand() {
  String x = String(line);
  Serial << line << endl;
  if      (x.startsWith(F("wifi"))) doConnect();
  else if (x.startsWith(F( "lt" ))) menuEnterColorRanges(trim(&line[2]));
  else if (x.startsWith(F("debug"))) switchDebugInfoPrint();
  else if (x.startsWith(F("reset"))) menuMainFactoryReset();
  else if (x.startsWith(F("tskey"))) saveLineToEE(trim(&line[5]), EE_40B_TSKEY);
  else if (x.startsWith(F("ubik")))  saveLineToEE(trim(&line[4]), EE_40B_UBIKEY);
  else if (x.startsWith(F("ubiv")))  saveLineToEE(trim(&line[4]), EE_40B_UBIVAR);
  else if (x.startsWith(F("test")))  sendToThingSpeak(567);
  else if (x.startsWith(F("brg"  ))) menuEnterLedBrightness(trim(&line[3]));
  else if (x.startsWith(F("gray" ))) switchGrayBox();
  else if (x.startsWith(F("sim"  ))) simulateCO2();
  else if (x.startsWith(F("ota"  ))) espOTA();
//  else if (x.startsWith(F("ping" ))) espPing();
  else if (x.startsWith(F("proxy"))) startSerialProxy();
  else if (x.startsWith(F("ppm"  ))) setPPM(trim(&line[3]));
  else if (x.startsWith(F("ppx"  ))) setPPX(trim(&line[3]));
//  else if (x.startsWith(F("esp"  ))) onlyESP();
  else if (x.startsWith(F("sap "))) EEPROM.put(EE_1B_HASSAPCFG, line[4]-'0');
  //else if (x.startsWith(F("beep"))) makeBeep();
  Serial << F(">") << endl;
}

void makeBeep() {
//  Serial << F("BEEP\n");
  
  for (int i=0; i<1; i++) {
    tone(A4, 200, 50);
    delay(200);
    tone(A4, 150, 50);
    delay(200);
    tone(A4, 100, 50);
    delay(200);
    tone(A4, 70, 50);
    delay(200);
  }

}


void saveLineToEE(const char *str, int address) {
  while (*str)  EEPROM.put(address++, (byte)*(str++));
  EEPROM.put(address, 0);
}

char* trim(const char *str) {
  while (*str == ' ') str++;
  return (char*)str;
}

void setPPM(char *val) {
  sPPM = String(val).toInt();
  processColors();
  oledCO2Level();
}

void setPPX(char *val) {
  int extPPM = String(val).toInt();
  double newMaxMv = getTGSEstMaxMv(extPPM, raCO2mv.getAverage());
  currentCO2MaxMv = newMaxMv;
  storeCurrentCO2MaxMv();  
}

//void onlyESP() {
//  pinMode(ESP_RX, INPUT);
//  pinMode(ESP_TX, INPUT);
//  espToggle();
//}

int menuEnterLedBrightness(const char *str) {
  //Serial  << line << ",," << String(line).toInt() << endl;
  int res = String(str).toInt();
  if (res < 255) res = constrain(res, 0, 150);
  overrideBrightness = res;
  Serial << F("New Brightness value: ") << overrideBrightness << endl;
  EEPROM.write(EE_1B_BRG, overrideBrightness);
  //menuHandler = menuPrintMain;
  return 0;
}

void menuEnterColorRanges(char *s) {
  storeColorRanges(s);
  processNeopixels();
  Serial << F("Done\n");
}

int menuMainFactoryReset() {
  clearEEPROM();
  Serial << F("\nConfiguration reset...\n");
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
}

int doConnect() {
  Serial << endl << F("Connecting to Wifi...") << endl;
  espToggle();
  if (!serialFind("ready", ESP_DEBUG, 10000)) {
    Serial << F("Wifi module not working") << endl;
  } else {
    Serial.flush();
    delay(1000);
    int res = setESPWifi(trim(&line[4]));
    if (res < 0) {
      Serial << F("Could not connect to Wifi: ") << res << endl;
    } else {
      setWifiStat("WiFi OK");
    }
  }
}

      
