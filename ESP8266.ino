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

boolean serialFind(char* keyword, boolean trace = false, int timeout = 2000) { 
  unsigned long deadline = millis() + timeout;
  while(millis() < deadline) {
    if (esp.available()) {
      char ch = esp.read();
      if (trace && (ch > 19 || ch == 10 || ch == 13) && ch < 128) Serial.write(ch);
      if (ch == *keyword && !*(++keyword)) return true;
    }
  }
  return false;  // Timed out
} 

boolean checkBaudRate(long b) {
  if (DEBUG) Serial << endl << F("Checking baud rate: ") << b << endl;
  esp.begin(b);
  espToggle();
  return serialFind("ready", DEBUG, 6000);
}

boolean espFixBaudRate() {
  if (DEBUG)  Serial << F("Fixing ESP Baudrate") << endl;
  if (checkBaudRate(9600)) return true;
  if (checkBaudRate(115200L) || checkBaudRate(74880L)) {
    esp << F("AT+UART_DEF=9600,8,1,0,0") << endl;
    serialFind(OK, DEBUG, 6000);
    espToggle();
    return checkBaudRate(9600);
  }
}

bool isWifiInit() {
  byte b;
  EEPROM.get(EE_1B_WIFIINIT, b);
  return b == 1;
}

int setESPWifiPass(const char *ssid, const char *pass) {
  EEPROM.put(EE_1B_WIFIINIT, 0);
  esp << F("AT+CWMODE_DEF=1") << endl;
  if (!serialFind(OK)) return -1;
  esp.flush();
  esp << F("AT") << endl;
  serialFind(OK, DEBUG, 1000);
  esp << F("AT+CWJAP_DEF=") << F("\"") << ssid << F("\"") << F(",") << F("\"") << pass << F("\"") << endl;
  if (!serialFind(GOTIP, DEBUG, 20000)) return -2;
  esp << F("AT+CWAUTOCONN=1") << endl;
  EEPROM.put(EE_1B_WIFIINIT, 1);
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
#define TS_GET F("GET /update?key=")
#define TS_GET_LEN 16
#define TS_FIELD F("&field1=")
#define TS_FIELD_LEN 8

int initESPForSending() {
  Serial << F("Starting Wifi Module...") << endl;
  espToggle();
  Serial << serialFind("ready", DEBUG, 3000) << endl;
  if (!serialFind("GOT IP", true, 30000)) {
    espOFF();
    return 0;
  } else {
    return 1;
  }
}

int sendTsInt(int value) {
  char key[30];
  EEPROM.get(EE_30B_TSKEY, key);
  
  Serial << F("TS KEY:") << key << endl;
  Serial << F("Sending to ThingSpeak") << endl;
  if (key[0] == 0 || key[1] == -1) return -4; //no tskey
  if (!initESPForSending()) return 0;
  Serial << endl;
  esp << F("AT+CIPSTART=\"TCP\",\"184.106.153.149\",80") << endl;
  if (!serialFind(OK, DEBUG, 4000)) return -1;
  
  int len = TS_GET_LEN + strlen(key) + TS_FIELD_LEN + String(value).length() + 2;
  esp << F("AT+CIPSEND=") << len << endl;
  if (!serialFind(">", DEBUG, 6000)) return -2;
  
  esp << TS_GET << key << TS_FIELD << value << endl << endl;
 // if (!serialFind(OK, true, 6000)) return -3;
  if (!serialFind("CLOSED", DEBUG, 6000)) return -4;
  return 1;
}


int sendToThingSpeak(int value) {
  int res = sendTsInt(value);
  if (res == 0) setWifiStat("No Wi-Fi");
  else if (res == 1) setWifiStat("OK");
  else if (res == -4) setWifiStat("No TS Key");
  else if (res < 0 ) setWifiStat("Failed Send");
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
  
  //Serial << F("TS RES:") << res << endl;
  espOFF();
  tmWifiSent = millis();
  
}

