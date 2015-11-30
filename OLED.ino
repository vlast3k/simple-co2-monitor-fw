#include "U8glib.h"
U8GLIB_SSD1306_128X64 u8g(6, 7, 8);		// HW SPI Com: CS = 10, A0 = 9 (Hardware Pins are  SCK = 13 and MOSI = 11)

byte contrast = 0;

void oledCO2Level() {
  if (dumpDebuggingInfo) return;
  u8g.setColorIndex(1);
  u8g.setContrast(255);
  u8g.firstPage();  

  do {
    if (sBrightness == 1) u8g.setFont(u8g_font_profont11r);
    else                  u8g.setFont(u8g_font_fub35n);
    u8g.setPrintPos(sPPM < 1000? 33: 5, 47);
    if (sPPM > 0) u8g << sPPM;
    u8g.setFont(u8g_font_profont11r);
    u8g.setPrintPos(0, 7);
    u8g << wifiStat;
    if (startedCO2Monitoring == false) {
      u8g.setPrintPos(70, 7);
      u8g << F("Warmup...");
      if (sPPM == 0 && millis() > 4000) {
        uint32_t sec = (CO2_FIRST_PROCESS_TIME - millis()) / 1000L;
        u8g.setFont(u8g_font_fub35n);
        u8g.setPrintPos(0, 47);
        u8g << sec;
        u8g.setFont(u8g_font_profont11r);
        u8g.setPrintPos(100, 47);
        u8g << F("sec");
      }
    }
   // u8g << "2292";
    

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
    u8g.setFont(u8g_font_profont11r);
   // u8g.drawFrame(0, 0, 128, 64);
    //u8g.drawLine(
    u8g.setPrintPos(1, 22);
    u8g << F("mv: ") << co2mv << F(", max: ") << co2maxmv;
    u8g.setPrintPos(1, 33);
    u8g << F("Tmp: ") << tmp << F(", ") << runtime;
    u8g.setPrintPos(1, 44);
    u8g << F("Brg: ") << sBrightness << F(", st: ") << startedCO2Monitoring;
    u8g.setPrintPos(1, 54);
    u8g << F("CO2: ") << sPPM;
    u8g.setPrintPos(1, 64);
    u8g << F("r");
    for (byte i=0; i < 5; i++) {
      EEPROM.get(EE_10B_TH + i*2, rng);
      u8g << rng/100 << " ";
    }


  } while( u8g.nextPage() );
}
