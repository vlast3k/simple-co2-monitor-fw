#ifdef TGS4161

#define REFTEMP 38.0D  //when the temperature at home is 22c, inside it is 38, so it is good middle point
#define TEMPCORR 0.2D
//#define TEMPCORR 0.6D
//TEMP CORR 0.44D
#define GAIN 5.7f

//#define PPM_AT_0 350.0d
//#define LOG_OF_PPM_AT_0 log10((double)PPM_AT_0)
//#define MV_AT_1000 (double)25.0d
//#define TG_OF_SLOPE (MV_AT_1000/(log10(1000) - LOG_OF_PPM_AT_0))
//#define GRADIENT 64.0

void testGradient() {
  int mv=30;
  for (int i=0; i < 100; i++) {
    Serial << "mv=" << mv << ", grad=" << i << ", ppm=" << mv2ppm(mv, i) << endl;
  }
}

void findAndStoreCorrectGradient(double mv, uint32_t realPPM) {
  uint8_t i=200;
  for (; i > 0 && mv2ppm(mv, i) < realPPM; i--);
  GRADIENT = i+1;
  EEPROM.put(EE_1B_GRADIENT, i);
}

void findGradient() {
  int mv=30;
  int ppmStd = mv2ppm(mv, GRADIENT);
  int ppmAdj = ppmStd*2;
  int i=200;
  for (; i > 0 && mv2ppm(mv, i) < ppmAdj; i--);
  Serial << "ppmStd=" << ppmStd << ", ppmAdj=" << ppmAdj << endl;
  Serial << "targetGrad mv=" << mv << ", grad=" << i << ", ppm=" << mv2ppm(mv, i) << endl;
  for (int k=0; k < 200;k++) {
    double d = mv2ppm(k, GRADIENT);
    d = (d-350)*2 + 350;
    Serial << "ppm*2=" << d << ", ppmGrad= " << mv2ppm(k, i) << endl;
  }
}

double tempAdjustMv(double mv) {
  double refTemp = (prevMaxMvTemp > 1) ? prevMaxMvTemp : currentMaxMvTemp;
  if (refTemp < 1) return mv;
  double diff = refTemp - raTempC.getAverage();
  if (diff > 4) diff = 4;
  else if (diff < -4) diff = -4;
  return mv + diff * 0.45F;
}

double getCO2_Mv(double v, boolean tcorr) {
  if (tcorr) {
    return tempAdjustMv(v/GAIN * 1000);
  } else {
    return v/GAIN * 1000;
  }
}

double getTGSEstMaxMv(int currPPM, double currMv) {
  return ppm2mv(currPPM, GRADIENT) + currMv;
}

#endif
