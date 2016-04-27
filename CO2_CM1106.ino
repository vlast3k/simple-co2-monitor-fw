#ifndef USELIB
#ifndef TGS4161
RunningAverage raCM1106(2);

byte getCS(byte* buf) {
  int sum=0;
  for (int i=0; i < buf[1]+2; i++) sum += buf[i];
  return (byte) 0xFF & (0x100 - sum);
}
boolean validateCS(byte* resp) {
  if (resp[0] != 0x16) {
    Serial << F("bad resp") << endl;
    return false;
  }
  if (resp[resp[1]+2] != getCS(resp)) {
    dump(resp);
    Serial << "bad cs:" << _HEX(getCS(resp)) << ", " << _HEX(resp[resp[1]+2]) << endl;  
    return false;
  }
}

boolean sendCmd(uint8_t *cmd, uint8_t *resp) {
  SoftwareSerial PM1106_swSer(SS_RX, SS_TX);
  PM1106_swSer.begin(9600);
  PM1106_swSer.write(&cmd[1], cmd[0]);
  delay(100);
  for (int i=0; i < 24 && PM1106_swSer.available(); i++) resp[i] = PM1106_swSer.read();
  PM1106_swSer.end();
  if (DEBUG) dump(resp);
  return validateCS(resp);
}

void co2Set400ppm() {
  Serial << F("Setting current CO2 level as 400 ppm\n");
  uint8_t cmdReadCO2[] = {6, 0x11, 0x03, 0x03, 1, 0x90, 0x58};
  uint8_t resp[30];
  Serial << sendCmd(cmdReadCO2, resp);
  Serial << "done" <<endl;
  
}

void checkForReset() {
  if (millis() > 7L*60*1000 && EEPROM.read(EE_1B_RESET_CO2) == 1) {
    EEPROM.write(EE_1B_RESET_CO2, 0);
    co2Set400ppm();
  }
}

int rawReadCM1106_CO2() {
  uint8_t cmdReadCO2[] = {4, 0x11, 0x01, 0x01, 0xed};
  uint8_t resp[30];
  sendCmd(cmdReadCO2, resp);
  uint16_t value = ((uint16_t)256)*resp[3] + resp[4];
  if (DEBUG) Serial << value << endl;
  raCM1106.addValue(value);
  return value;  
}

uint32_t lastNDIRRead = 0;
#define NDIR_READ_TIMEOUT 10000L
int CM1106__getCO2() {
  if (timePassed(lastNDIRRead, NDIR_READ_TIMEOUT) == false) return sPPM;
  checkForReset();
  lastNDIRRead = millis();
  //Serial <<"reading\n";
  uint16_t value = rawReadCM1106_CO2();
  if (value == -1) return 0;
  else if ((millis() < 130L*1000) && (value == 550)) {
    startedCO2Monitoring = false;
    return 0;
  } else {
    startedCO2Monitoring = true;
    return (int)raCM1106.getAverage();
  }
  //Serial << "co2: " << (int)raCM1106.getAverage() << endl;
  
}

void dump(uint8_t *r) {
  for (int i=0; i<24; i++) {
    Serial.print(*(r++), HEX);
    Serial.print(",");
  } 
  Serial.println();
}

void debugInfoCM1106() {
  Serial << "CM1106 raw: " << rawReadCM1106_CO2() << endl;
}

#endif
#endif
