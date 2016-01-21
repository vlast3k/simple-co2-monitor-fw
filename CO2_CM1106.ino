#ifndef TGS4161
RunningAverage raCM1106(5);

uint32_t lastNDIRRead = 0;
#define NDIR_READ_TIMEOUT 10000L
int CM1106__getCO2() {
  if (timePassed(lastNDIRRead, NDIR_READ_TIMEOUT) == false) return sPPM;
  lastNDIRRead = millis();
  //Serial <<"reading\n";
  SoftwareSerial PM1106_swSer(SS_RX, SS_TX);
  uint8_t cmdReadCO2[] = {4, 0x11, 0x01, 0x01, 0xed};
  uint8_t resp[30];

  PM1106_swSer.begin(9600);
  PM1106_swSer.write(&cmdReadCO2[1], cmdReadCO2[0]);
  delay(100);
  for (int i=0; i < 24 && PM1106_swSer.available(); i++) resp[i] = PM1106_swSer.read();
  //dump(resp);
  uint16_t value = ((uint16_t)256)*resp[3] + resp[4];
  raCM1106.addValue(value);
  PM1106_swSer.end();
  if ((millis() < 130L*1000) && (value == 550)) {
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

#endif
