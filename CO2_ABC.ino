
uint32_t CO2_FIRST_PROCESS_TIME; //=  60L*15*1000  //how much to wait before first storing of the data usually 15 min
//#define CO2_FIRST_PROCESS_TIME 60L*15*1000  //how much to wait before first storing of the data usually 15 min

int cfg_lowest_co2_ppm = 400;
byte cfg_abc_resetHours = 120;

unsigned long lastEEPROMWrite = 0;
unsigned long lastCO2Read = 0;


#define ONE_HOUR 60L*60*1000

unsigned long timeOneHourStarted = 0;

void initCO2ABC() {
  if (!DEBUG) {
    CO2_FIRST_PROCESS_TIME = 60L*15*1000;
  } else {
    CO2_FIRST_PROCESS_TIME = 15*1000;
  }
  EEPROM.get(EE_FLT_CURRENT_PERIOD_CO2_HIGHESTMV, currentCO2MaxMv);
  if (isnan(currentCO2MaxMv)) currentCO2MaxMv = 0;
  EEPROM.get(EE_FLT_PREV_PERIOD_CO2_HIGHESTMV, prevCO2MaxMv);
  if (isnan(prevCO2MaxMv)) prevCO2MaxMv = 0;
}

void processCO2() {
//  if (timePassed(lastCO2Read, CO2_READ_TIMEOUT) == false) return;
//  lastCO2Read = millis();

  readSensorData();
  
  if (startedCO2Monitoring || (millis() > CO2_FIRST_PROCESS_TIME)) {  
    //once the monitoring starts - rollover will not play
    processCO2SensorData();
    if (!startedCO2Monitoring) {
      storeCurrentCO2MaxMv();
      startedCO2Monitoring = true;  
    }
  }
  computeCO2PPM();

  if (timePassed(timeOneHourStarted, ONE_HOUR)) {
  //if (timePassed(timeOneHourStarted, 20000L)) {
    timeOneHourStarted = millis();
    co2OnOneHour();
  }
}

void readSensorData() {
  raTempC.addValue(getTermistorC(getVolts(1023 - analogReadFine(TEMP_PIN, ANALOG_READ_PRECISION))));
  double dd = getVolts(analogReadFine(CO2_PIN, ANALOG_READ_PRECISION));
  raCO2mv.addValue(getCO2_Mv(dd, true)); 
  raCO2mvNoTempCorr.addValue(getCO2_Mv(dd, false));
}

void processCO2SensorData() {
  //the default ppm is 350, that is - we need to adjust the values as if they had hit the max ppm of 350. We assume that 400 ppm is the lowest that can be received
  double co2MvAdj = raCO2mv.getAverage() + ppm2mv((double)cfg_lowest_co2_ppm);
  if (co2MvAdj > currentCO2MaxMv) {
    //update the current max CO2, and store it. Since the Read Timeout is 60 seconds, this is will ensure that there will be only a limited stores to the Flash memory until the treshold is reached. Maximum should be 5000 times, until the sensor warms for a few days, and only if it sits outside
    currentCO2MaxMv = co2MvAdj;
  }
}

void computeCO2PPM() {
  sPPM = mv2ppm(getCO2MaxMv() - raCO2mv.getAverage());
}

void storeCurrentCO2MaxMv() {
  EEPROM.put(EE_FLT_CURRENT_PERIOD_CO2_HIGHESTMV, currentCO2MaxMv);  
}

void co2OnOneHour() {
  storeCurrentCO2MaxMv();
  if (eeAddHourAndReturn() == cfg_abc_resetHours) {
    for (byte i=0; i < 4; i++) EEPROM.put(EE_4B_HOUR + i, (byte)0);
    EEPROM.put(EE_FLT_PREV_PERIOD_CO2_HIGHESTMV, currentCO2MaxMv);
    prevCO2MaxMv = currentCO2MaxMv;
    currentCO2MaxMv = 0;
  }
}


double getEECurrentCO2MaxMv() {
  double d;
  EEPROM.get(EE_FLT_CURRENT_PERIOD_CO2_HIGHESTMV, d);
  return d;
}

double getEEPrevCO2MaxMv() {
  double d;
  EEPROM.get(EE_FLT_PREV_PERIOD_CO2_HIGHESTMV, d);
  return d;
}

double getCO2MaxMv() {
  return max(prevCO2MaxMv, currentCO2MaxMv);
}

byte eeGetHours() {
  byte maxH = 0, pos = 0, r = 0, i = 0;
  for (; i < 4; i++) {
    if (EEPROM.get(i + EE_4B_HOUR, r) && r > maxH && r != 255) {
      maxH = r;
      pos = i;
    }
  }
  return maxH;
}

byte eeAddHourAndReturn() {
  byte maxH = 0, pos = 0, r = 0, i = 0;
  for (; i < 4; i++) {
    if (EEPROM.get(i + EE_4B_HOUR, r) && r > maxH && r != 255) {
      maxH = r;
      pos = i;
    }
  }
  EEPROM.put(EE_4B_HOUR + ((i+1) % 4), maxH + 1);
  return maxH + 1;
}

void debugInfoCO2ABC() {
  Serial << F("currCO2Mv   :") << raCO2mv.getAverage() << endl;
  Serial << F("currCO2MvNoT:") << raCO2mvNoTempCorr.getAverage() << endl;
  Serial << F("diff        :") << raCO2mvNoTempCorr.getAverage() - raCO2mv.getAverage() << endl;
  Serial << F("currCO2MaxMv:") << currentCO2MaxMv << endl;
  Serial << F("prevCO2MaxMv:") << prevCO2MaxMv << endl;
  Serial << F("temp        :") << raTempC.getAverage() << endl;
  Serial << F("uptime      :") << millis2min() << endl;
  Serial << F("hours       :") << eeGetHours() << endl;
  Serial << F("sPPM        :") << sPPM << endl;     
}



