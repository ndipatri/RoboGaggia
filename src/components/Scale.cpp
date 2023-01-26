
#include "Scale.h"

#include <Qwiic_Scale_NAU7802_Arduino_Library.h>

ScaleState scaleState;

NAU7802 myScale; //Create instance of the NAU7802 class

// This is the suggested target weight when in
// MEASURE_BEANS state
double TARGET_BEAN_WEIGHT = 19; // grams

// The extraction weight which triggers the end of PREINFUSION
int PREINFUSION_WEIGHT_THRESHOLD_GRAMS = 1;

// This is the ration of final espresso weight compared to
// that of the ground beans.  Typically this is 2-to-1
float BREW_WEIGHT_TO_BEAN_RATIO = 2.0;

// Spreadsheet which shows calibration for this scale:
// https://docs.google.com/spreadsheets/d/1z3atpMJ9mnRWZPx-S3QPc0Lp2lXNPtheInoUFgV0nMo/edit?usp=sharing 
// weightInGrams = scaleReading * SCALE_FACTOR + SCALE_OFFSET
// y = 6.33E-04*x + -13.2
double SCALE_FACTOR = 0.000633; // 3137;  
double SCALE_OFFSET = -13.2;  // 47;

// Below which we consider weight to be 0
// Ideally if scale is calibrated properly and has solid connections, this
// shouldn't be needed.
int LOW_WEIGHT_THRESHOLD = 0;

void readScaleState() {
  scaleState.measuredWeight = 0;

  if (myScale.available() == true) {
    float scaleReading = myScale.getReading();

    float weightInGrams = scaleReading * SCALE_FACTOR + SCALE_OFFSET;

    // This is a rotating buffer
    scaleState.avgWeights[scaleState.avgWeightIndex++] = weightInGrams;
    if(scaleState.avgWeightIndex == SCALE_SAMPLE_SIZE) scaleState.avgWeightIndex = 0;

    float avgWeight = 0;
    for (int index = 0 ; index < SCALE_SAMPLE_SIZE ; index++)
      avgWeight += scaleState.avgWeights[index];
    avgWeight /= SCALE_SAMPLE_SIZE;

    scaleState.measuredWeight = avgWeight;
  }
}

void scaleInit() {
  // Scale check
  if (myScale.begin() == false)
  {
    Log.error("Scale not detected!");
  }

  myScale.setGain(NAU7802_GAIN_64); //Gain can be set to 1, 2, 4, 8, 16, 32, 64, or 128. default is 16
  myScale.setSampleRate(NAU7802_SPS_80); //Sample rate can be set to 10, 20, 40, 80, or 320Hz. default is 10
  myScale.calibrateAFE(); //Does an internal calibration. Recommended after power up, gain changes, sample rate changes, or channel changes.

  Particle.variable("tareWeightGrams",  scaleState.tareWeight);
  Particle.variable("measuredWeightGrams",  scaleState.measuredWeight);
}


