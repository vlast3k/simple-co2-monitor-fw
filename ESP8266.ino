char OK[] = "OK";
char GOTIP[] = "GOT IP";


void espON() {
  Serial << " ESP Turn on " << endl;
  esp.begin(9600);
  pinMode(ESP_CHPD, OUTPUT);
  digitalWrite(ESP_CHPD, HIGH);
  espIsOn = true;
  espSentInit = false;
  espLastActivity = millis();
}

void espOFF() {
  Serial << " ESP Turn OFF " << endl;
  esp.end();
  pinMode(ESP_CHPD, OUTPUT);
  digitalWrite(ESP_CHPD, LOW);
  espIsOn = false;
}

void espToggle() {
  espOFF();
  delay(1000);
  espON();
}

void espSend(const char *s) {
  espLastActivity = millis();
  esp.write(s);
  esp.flush();
}

int startSerialProxy() {
  Serial << F("ESP Proxy\n");
  espToggle();
  for (;;) {
    if (esp.available())    Serial.write(esp.read());
    if (Serial.available()) esp.write(Serial.read());
  }
  return 0;
}

int lastCO2 = 0;

#ifdef TGS4161
void sendNow() {
  char s[100];
  if (sPPM == 0) return;
  sprintf(s, "sendNow CO2,%ld,MVNTC,%d.%d,MV,%d.%d,CURMAXMV,%d.%d,TGTEMP,%d.%d,LIGHT,%d\r\n", sPPM,
          (int)raCO2mvNoTempCorr.getAverage(), getFloat(raCO2mvNoTempCorr.getAverage()),
          (int)raCO2mv.getAverage(), getFloat(raCO2mv.getAverage()),
          (int)currentCO2MaxMv, getFloat(currentCO2MaxMv),
          (int)raTempC.getAverage(), getFloat(raTempC.getAverage()),
          (int)raLight.getAverage());
   espSend(s);  
}
#else
void sendNow() {
  char s[100];
  sprintf(s, "sendNow CO2,%d,LIGHT,%d\r\n", sPPM, (int)raLight.getAverage());
  espSend(s);  
}
#endif

uint32_t tmWifiSent = 0;
void processSendData() {
  if (!hasESP) return;
//  if (!())  {
//    setWifiStat("Setup Wifi");
//    return;
//  }
  if (sPPM == 0) return;
  int16_t wifiSendInterval;
  EEPROM.get(EE_2B_WIFI_SND_INT_S, wifiSendInterval);
  if (wifiSendInterval <= 1) wifiSendInterval = 120;
  if (!timePassed(tmWifiSent, 1000L * wifiSendInterval)) return;
  if (espIsOn) sendNow();
  else {
    espToggle();
    espWasTurnedOff = true;
  }
  //int res = sendToThingSpeak(sPPM);
  
//  Serial << endl << F("TS RES: ") << res << endl;
  tmWifiSent = millis();
  
}

