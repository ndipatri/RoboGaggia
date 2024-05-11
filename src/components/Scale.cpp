
#include "Scale.h"

#include <SparkFun_Qwiic_Scale_NAU7802_Arduino_Library.h>

ScaleState scaleState;

NAU7802 myScale; //Create instance of the NAU7802 class

float KNOWN_CUP_WEIGHT_GRAMS = 54.8;

// This is the suggested target weight when in
// MEASURE_BEANS state
double TARGET_BEAN_WEIGHT = 19; // grams

// The extraction weight which triggers the end of PREINFUSION
int PREINFUSION_WEIGHT_THRESHOLD_GRAMS = 4;

// This is the ration of final espresso weight compared to
// that of the ground beans.  Typically this is 2-to-1
float BREW_WEIGHT_TO_BEAN_RATIO = 2.0;

// This assumes the scale has been properly zero'd and calibrated using
// below functions.
// At 40 SPS, 20 Samples means it takes about 500ms to take this reading.
void readScaleState() {
  if (myScale.available() == true) {
    // allow negative values, tell scale to average values over 20 sample periods...
    scaleState.measuredWeight = myScale.getWeight(true, 20); 
  } else {
    Log.error("Scale not detected!");
  }
}

// This assumes the reference weight is on the scale 
// With current settings this is blocking at takes about 1.6 seconds. (40 SPS/64 samples)
void calibrateScale()
{
  // Tell the library how much weight is currently on it
  // We are sampling slowly, so we need to increase the timeout too
  myScale.calculateCalibrationFactor(KNOWN_CUP_WEIGHT_GRAMS, 64, 3000); //64 samples at 40SPS. Use a timeout of 3 seconds

  // Now that this is done, we can make 'getWeight() in grams' calls on the scale instead of
  // unitless getReading() calls! 
}

void zeroScale() {
  //Perform an external offset - this sets the NAU7802's internal offset register
  myScale.calibrateAFE(NAU7802_CALMOD_OFFSET); //Calibrate using external offset
}

// This assumes nothing is currently on the scale
void scaleInit() {
  // Scale check
  if (myScale.begin() == false)
  {
    Log.error("Scale not detected!");
  }

  myScale.setSampleRate(NAU7802_SPS_40); //Set sample rate: 10, 20, 40, 80 or 320
  myScale.setGain(NAU7802_GAIN_16); //Gain can be set to 1, 2, 4, 8, 16, 32, 64, or 128.
  myScale.setLDO(NAU7802_LDO_3V0); //Set LDO (AVDD) voltage. 3.0V is the best choice for Qwiic

  myScale.setCalibrationFactor(1.0);
  myScale.setChannel1Offset(0);
}