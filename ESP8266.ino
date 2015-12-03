char OK[] = "OK";
char GOTIP[] = "GOT IP";

void espON() {
  pinMode(ESP_CHPD, OUTPUT);
  digitalWrite(ESP_CHPD, HIGH);
}

void espOFF() {
  pinMode(ESP_CHPD, OUTPUT);
  digitalWrite(ESP_CHPD, LOW);
}

void espToggle() {
  espOFF();
  delay(1000);
  espON();
}

boolean serialFind(char* keyword, boolean trace = false, unsigned long timeout = 2000) { 
  char *k = keyword;
  unsigned long deadline = millis() + timeout;  
  while(millis() < deadline) {
    if (esp.available()) {
      char ch = esp.read();
      if (trace && (ch > 19 || ch == 10 || ch == 13) && ch < 128) Serial.write(ch);
      if (ch == *k) {
        if (!*(k+1)) return true;
        else k++;
      } else {
        k = keyword;
      }
    }
  }
  return false;  // Timed out
} 

boolean isWifiInit() {
  return EEPROM.read(EE_1B_WIFIINIT) == 1;
}

boolean isSAPAuth(const char *str) {
  //count number of " to see if there are 3 words
  int n = 0;
  while (*str) if (*(str++) == '\"') n++;
  return n == 6;
}

int setESPWifi(const char *str) {
  EEPROM.put(EE_1B_WIFIINIT, 0);
  esp << F("AT+CWMODE_DEF=1") << endl;
  if (!serialFind(OK, ESP_DEBUG, 10000)) return -1;
  esp.flush();
  esp << F("AT") << endl;
  serialFind(OK, ESP_DEBUG, 1000);
  esp << F("AT+CWJAP_DEF=") << str << endl;
  if (isSAPAuth(str)) {
    if (!serialFind("Auth - OK!", ESP_DEBUG, 20000)) return -2;
  } else {
    if (!serialFind(GOTIP, ESP_DEBUG, 20000)) return -2;
    esp << F("AT+CWAUTOCONN=1") << endl;
  }
  EEPROM.put(EE_1B_WIFIINIT, 1);
  Serial << F("WIFI OK!") << endl;
  return 1;  
}

int startSerialProxy() {
  Serial << F("ESP Proxy") << endl;
  espToggle();
  for (;;) {
    if (esp.available())    Serial.write(esp.read());
    if (Serial.available()) esp.write(Serial.read());
  }
  return 0;
}

#define TS_IP F("184.106.153.149")
#define UBI_IP F("50.23.124.66")

boolean initESPForSending() {
  Serial << F("Starting Wifi Module...") << endl;
  espToggle();
  Serial << serialFind("ready", ESP_DEBUG, 3000) << endl;
  if (!serialFind("GOT IP", true, 30000)) {
    espOFF();
    return false;
  } else {
    Serial <<"FOUND" << endl;
    return true;
  }
}

int lastCO2 = 0;
boolean makeGETRequestTS(char *s, int value) {
  char key[30];
  EEPROM.get(EE_40B_TSKEY, key);
  
  Serial << F("TS KEY:") << key << endl;
  if (key[0] == 0 || key[0] == -1) return false; //no tskey
  
  sprintf(s, "GET /update?key=%s&field1=%d&field2=%d.%d&field3=%d.%d&field4=%d.%d&field5=%d.%d&field6=%d\n\n", key, value,
   (int)raCO2mvNoTempCorr.getAverage(), getFloat(raCO2mvNoTempCorr.getAverage()),
   (int)raCO2mv.getAverage(), getFloat(raCO2mv.getAverage()),
   (int)currentCO2MaxMv, getFloat(currentCO2MaxMv),
   (int)raTempC.getAverage(), getFloat(raTempC.getAverage()),
   lastCO2 ? lastCO2 - value: 0);  

   return true;
}

boolean makeGETRequestUBI(char *s, int value) {
  char key[40], var[30];
  EEPROM.get(EE_40B_UBIKEY, key);
  EEPROM.get(EE_40B_UBIVAR, var);
  Serial << "UBI:" << key << "," << var << endl;
  if (key[0] == 0 || key[0] == -1) return false; //no tskey
  sprintf(s, "GET /api/postvalue/?token=%s&variable=%s&value=%d\n\n", key, var, value);
  Serial << F("UBI: ") << s << endl;
  return true;
}

boolean makeGETRequestSAP(char *s, int value) {
  if (EEPROM.read(EE_1B_HASSAPCFG) != 1) return false;
  sprintf(s, "sndiot %d\n\n", value);
  Serial << F("SAP: ") << s << endl;
  return true;
}

int sendTsInt(int value, int endpoint) {
  char sendstr[150];
  if (endpoint == 1 && !makeGETRequestTS(sendstr, value)) return -1;
  else if (endpoint == 2 && !makeGETRequestUBI(sendstr, value)) return -1;
  else if (endpoint == 3 && !makeGETRequestSAP(sendstr, value)) return -1;

  int i;
  for (i=0; i < 7; i++) {
    esp << F("AT+CIPSTART=\"TCP\",\"")<< (endpoint == 1 ? TS_IP : UBI_IP) << F("\",80") << endl;
    if (!serialFind(OK, true, 4000)) continue;
    
    esp << F("AT+CIPSEND=") << strlen(sendstr) << endl;
    if (!serialFind(">", true, 6000)) continue;
  
    esp << sendstr;
    if(serialFind("CLOSED", true, 6000)) break;
  }
    
  return i < 7;
}


int sendToThingSpeak(int value) {
  int res, res2;
  
  if (!initESPForSending() && !initESPForSending()) res = 0; // try to connect to Wifi 2x
  else {
    res = sendTsInt(value, 1);
    res2 = sendTsInt(value, 2);
    res2 = sendTsInt(value, 3);
  }
  espOFF();
  lastCO2 = value;
  if (res == 0) setWifiStat("No Wi-Fi");
  else if (res == 1) setWifiStat("OK");
  else if (res == -1) setWifiStat("No TS Key");
  else if (res < -1 ) setWifiStat("Error");
  return res;  
}

uint32_t tmWifiSent = 0;
#define WIFI_SEND_INTERVAL 120L*1000
void processSendData() {
  if (!isWifiInit())  {
    setWifiStat("Setup Wifi");
    return;
  }
  if (!timePassed(tmWifiSent, WIFI_SEND_INTERVAL)) return;


  int res = sendToThingSpeak(sPPM);
  
  Serial << endl << F("TS RES: ") << res << endl;
  espOFF();
  tmWifiSent = millis();
  
}

int espOTA() {
  Serial << F("OTA Start") << endl;
  if (!initESPForSending()) {
    Serial << F("init failed") << endl;
    return 1;
  }
  esp << F("o") << endl;
  serialFind("artyy", true, 300000L);
  Serial << F("espOta: exit") << endl;
  return 1;
}


int espPing() {
  Serial << F("Ping ") << endl;
  if (!initESPForSending()) {
    Serial << F("init failed") << endl;
    return 1;
  }
  esp << F("p\r\n") << endl;
  serialFind("CLOSED", true, 300000L);
  return 1;
}

