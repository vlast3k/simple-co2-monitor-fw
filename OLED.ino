#include "U8glib.h"
U8GLIB_SSD1306_128X64 u8g(6, 7, 8);		// HW SPI Com: CS = 10, A0 = 9 (Hardware Pins are  SCK = 13 and MOSI = 11)

void displaySensorsOLED() {
  u8g.setColorIndex(1);
  u8g.firstPage();  
  do {
    u8g.setFont(u8g_font_profont12r);
    u8g.setPrintPos(1, 10);
    u8g << F("CO2: ") << sPPM << F(" ppm");
    u8g.setPrintPos(1, 30);
    u8g << F("Temp: ") << raTempC.getAverage() << F("C");
    u8g.setPrintPos(1,50);
    u8g << F("Brg: ") << sBrightness;
  } while( u8g.nextPage() );

}

void displayWelcome() {
  u8g.setColorIndex(1);
  u8g.firstPage();  
  do {
    u8g.setFont(u8g_font_profont12r);
    u8g.drawStr( 1, 10, F("WiFi CO2 Meter"));
    u8g.drawStr( 1, 30, F("by Vladimir Savchenko"));
    u8g.drawStr( 1, 50, F("February 2015"));
  } while( u8g.nextPage() );
} 

void oledAll() {
  u8g.setColorIndex(1);
  u8g.firstPage();  
  do {
    u8g.setFont(u8g_font_profont12r);
    u8g.setPrintPos(0, 7);
    u8g << F("OOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOO");
    u8g.setPrintPos(0, 64);
    u8g << F("OOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOO");
    

  } while( u8g.nextPage() );
}
void oledTechnicalDetails() {
  u8g.setColorIndex(1);
  u8g.firstPage();  
  String runtime = millis2min();
  int co2mv = (int)raCO2mv.getAverage();
  int co2maxmv = (int)(getCO2MaxMv() - ppm2mv((double)cfg_lowest_co2_ppm));
  byte tmp = (byte)raTempC.getAverage();
    int rng;
  do {
    u8g.setFont(u8g_font_profont12r);
    u8g.setPrintPos(1, 10);
    u8g << F("mv: ") << co2mv << F(", max: ") << co2maxmv;
    u8g.setPrintPos(1, 22);
    u8g << F("Tmp: ") << tmp << F(", ") << runtime;
    u8g.setPrintPos(1, 34);
    u8g << F("Brg: ") << sBrightness << F(", st: ") << startedCO2Monitoring;
    u8g.setPrintPos(1, 46);
    u8g << F("CO2: ") << sPPM;
    u8g.setPrintPos(1, 58);
    u8g << F("r");
    for (byte i=0; i < 5; i++) {
      EEPROM.get(EE_10B_TH + i*2, rng);
      u8g << rng/100 << " ";
    }


  } while( u8g.nextPage() );
}
