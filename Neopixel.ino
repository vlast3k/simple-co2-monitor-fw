
#define COL1 pixels.Color(0,255,0)
#define COL2 pixels.Color(140,255,0)
#define COL3 pixels.Color(255,255,0)
#define COL4 pixels.Color(255,200,0)
#define COL5 pixels.Color(255,120,0)
#define COL6 pixels.Color(255,0,0)
#define BLUE pixels.Color(0,0,255)
uint32_t colors[] = {COL1, COL2, COL3, COL4, COL5, COL6};
#define DEFAULT_COLOR_RANGES "700 1000 1300 1600 1900 "
//uint16_t colorRanges[] = {700, 1000, 1300, 1600, 1900};
byte maxBrightness = 120;
uint32_t forcedColorMS = 0;

byte ppm2idx(uint32_t ppm) {
  int rng;
  if (ppm > 6000) return 1;
  else if (ppm > 10000) return 2;
  else if (ppm > 20000) return 3;
  else if (ppm > 40000L) return 4;
  else if (ppm > 80000L) return 5;
  else if (ppm > 160000L) return 6;
  for (byte i=0; i < 5; i++) {
    EEPROM.get(EE_10B_TH + i*2, rng);
    if (ppm < rng) return i;
  }
  return 5;
}

uint32_t idx2color(byte idx, uint32_t ppm) {
  if (ppm > 6000) return BLUE;
  return colors[idx];
}

void storeColorRanges(char *p) {
  String num;
  byte idx = 0;
  if (*p == 'd') storeColorRanges(DEFAULT_COLOR_RANGES);
  else {
    while (idx < 5) {
      if (isDigit(*p)) num += *p;
      else {
        EEPROM.put(EE_10B_TH + (idx++ * 2), atoi(num.c_str()));
        num = "";
      }
      if (!*p) break;
      p++;
    }
    
  }
}



void initColorRanges() {
  int rng;
  EEPROM.get(EE_10B_TH, rng);
  if (rng == -1) storeColorRanges(DEFAULT_COLOR_RANGES);
}

void initNeopixels() {
  initColorRanges();
  pixels.begin();
  for (int i=0; i< 6; i++) pixels.setPixelColor(i, colors[i]);
  pixels.setBrightness(20);
  pixels.show();
}

void debugInfoNeopixel() {
  Serial << F("Brightness  :") << sBrightness<< endl;
}
float getBrgFactor() {
  byte bb = EEPROM.read(EE_1B_BRG_FACTOR);
  return (float)(bb==255?10:bb) / 10;
  
}
#define BRG_CHECK_TIMEOUT 10000
uint32_t lastBrgCheck = 0;
void processBrightness() {
  if (timePassed(lastBrgCheck, BRG_CHECK_TIMEOUT) == false) return;
  lastBrgCheck = millis();
  if (overrideBrightness == 255) { 
    raLight.addValue(analogReadFine(LIGHT_PIN, 1));
    int maxLightRead = EEPROM.read(EE_1B_ISGRAY) == 1 ? 400 : 1000;
    int light = (int)((float)raLight.getAverage() * getBrgFactor());
    //Serial << "ralig: " << raLight.getAverage() << ", brg:" << getBrgFactor() << endl;
    int r=0;
    if (light <= 10) {
      sBrightness = light;
      if (!sBrightness) sBrightness = 1;
    } else {
      r = constrain(light, 0, maxLightRead);
      sBrightness = map(r, 11, maxLightRead, 11, maxBrightness);
    }
    //Serial << maxLightRead << "." << r << "." << sBrightness << "." << maxBrightness << endl;
  } else {
    sBrightness = overrideBrightness;
   //Serial << "ldr: " << analogReadFine(LIGHT_PIN, 1) << endl;
  }
  pixels.setBrightness(sBrightness);
  pixels.show();
}

void ledHeat(bool turnOn) {
  pixels.begin();
  for (int i=0; i< 6; i++) {
    if (turnOn) pixels.setPixelColor(i, pixels.Color(255, 255, 255));
    else pixels.setPixelColor(i, pixels.Color(0, 0, 0));
  }
  //pixels.setBrightness(20);
  pixels.show();
  
}

uint32_t lastNeoPixelChange = 0;
#define NEOPIXEL_TIMEOUT 1000L
#define FORCEDLED_TIMEOUT 60000L

void processColors() {
  if (!sPPM) return;
  byte i = 0;
  int adj = 0;
  if (!timePassed(forcedColorMS, FORCEDLED_TIMEOUT)) adj = -1;
  Serial << "adj: " << adj << endl;
  if (sPPM == -1) {
    for (; i < NUMPIXELS + adj     ; i++) pixels.setPixelColor(i, 0);
    pixels.setPixelColor(0, pixels.Color(0, 0, 255));
    pixels.show();
    return;
  }
  byte idx = ppm2idx(sPPM);
  uint32_t color = idx2color(idx, sPPM);
  for (; i <= idx + adj     ; i++) pixels.setPixelColor(i, color);
  for (; i < NUMPIXELS + adj; i++) pixels.setPixelColor(i, 0);
  pixels.show(); 
}

//void overrideLedsMax() {
//  int col = 5;
//  for (int i=0; i < col   ; i++) pixels.setPixelColor(i, colors[col-1]);
//  //pixels.setBrightness(200);
//  pixels.show();
//  
//}



void processNeopixels() {
  if (timePassed(lastNeoPixelChange, NEOPIXEL_TIMEOUT) == false) return;
  lastNeoPixelChange = millis();
  // Serial << "neopixels" << endl;
//  if (overrideLeds) {
//    overrideLedsMax();
//       processBrightness();
//  } else {
    processColors();
    processBrightness(); 
 // }
}
void onLED(String &x) {
  int a = x.indexOf(F("LED##")) + 5;
  Serial << a << endl;
  Serial << line + a << endl;
  const char *p = strchr(line, ' ');
  int r = atoi(p);
  p = strchr(p + 1, ' ');
  int g = atoi(p);
  p = strchr(p + 1, ' ');
  int b = atoi(p+1);
  Serial <<"LED: " << r << "," << g << "," << b << endl;
  pixels.setPixelColor(5, pixels.Color(r, g, b));
  pixels.show();
  forcedColorMS = millis();
}

