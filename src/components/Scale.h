#ifndef SCALE_H
#define SCALE_H

#include "Heater.h"
#include "Scale.h"
#include "WaterPump.h"
#include "Network.h"

#define SCALE_SAMPLE_SIZE 20 

extern int PREINFUSION_WEIGHT_THRESHOLD_GRAMS;

struct ScaleState {

  // The current weight measurement is a sliding average
  double avgWeights[SCALE_SAMPLE_SIZE];
  byte avgWeightIndex = 0;

  double measuredWeight = 0.0;

  // this will be the measuredWeight - tareWeight * BREW_WEIGHT_TO_BEAN_RATIO
  // at the moment this value is recorded...
  double targetWeight = 0; 

  // recorded weight of cup meant be used when
  // measuring the weight of beans or brew  
  double tareWeight = 0;
};

extern ScaleState scaleState;

void scaleInit();

void zeroScale();

void calibrateScale();

void readScaleState();

#endif