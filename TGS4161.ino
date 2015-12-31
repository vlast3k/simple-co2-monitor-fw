#define REFTEMP 38.0D  //when the temperature at home is 22c, inside it is 38, so it is good middle point
#define TEMPCORR 0.6D
//#define TEMPCORR 0.0D
#define GAIN 5.7f

#define MV_AT_1000 (double)25.0d
#define PPM_AT_0 350.0d
#define LOG_OF_PPM_AT_0 log10((double)PPM_AT_0)
#define LOG_OF_350 log10(350.0d)
#define TG_OF_SLOPE (MV_AT_1000/(log10(1000) - LOG_OF_PPM_AT_0))
#define GRADIENT 64.0
//#define GRADIENT 71.0
int mv2ppm(double mv) { return pow((double)10, mv / GRADIENT + LOG_OF_350); }
double ppm2mv(int ppm) { return (log10((double)ppm) - LOG_OF_350) * GRADIENT; }

double tempAdjustMv(double mv) {
//  return mv;
  //it seems like temeperatures smaller than 18.5 degrees do not affect the sensor
  return mv + (REFTEMP - max(raTempC.getAverage(), 18.5))*TEMPCORR;
}

double getCO2_Mv(double v, boolean tcorr) {
  if (tcorr) {
    return tempAdjustMv(v/GAIN * 1000);
  } else {
    return v/GAIN * 1000;
  }
}

double getTGSEstMaxMv(int currPPM, double currMv) {
  return ppm2mv(currPPM) + currMv;
}

