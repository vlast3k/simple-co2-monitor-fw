boolean timePassed(unsigned long since, unsigned long interval) {
  return (unsigned long)(millis() - since) > interval;
}

double getVolts(double pinValue) {
   return pinValue * VIN / 1024;
}

//String millis2min() {
//  int minutes = millis()/1000/60;
//  String res;
//  if (minutes/60) res += String(minutes/60) +"h ";
//  res += String(minutes % 60) + "m ";
//  res += String((millis() / 1000) %60) + "s";
//  return res;
//}

double getTermistorC(double volts) {
  double r = (10*volts)/(VIN-volts)*1000;
  double steinhart;
  steinhart = r / 10000.0f;     // (R/Ro)
  steinhart = log(steinhart);                  // ln(R/Ro)
  steinhart /= 3380;                   // 1/B * ln(R/Ro)
  steinhart += 1.0 / (25 + 273.15); // + (1/To)
  steinhart = 1.0 / steinhart;                 // Invert
  steinhart -= 273.15;                         // convert to C
  
  return steinhart;
}

double analogReadFine(int pin, byte prec = 30) {
  unsigned long sum = 0;
  for (long i=0; i < ((long)prec) * 1000; i++) { 
    sum += analogRead(pin);
   // delay(1);
  }
  return (double)sum / (prec * 1000);
}

void softwareReset() {
  Serial << F("\nRestarting...\n");
  delay(300);
  asm volatile ("  jmp 0");  
}  

void clearEEPROM() {
  for (int i=0; i < 100; i++) EEPROM.write(i, 255);
}

int getFloat(float f) {
  return (f - (int)f)*10;
}



