
#ifndef TELEMETRY_H
#define TELEMETRY_H

#include "Network.h"
#include "Common.h"
#include "State.h"

// This contains both instantaneous metrics (e.g. measuredWeight) and other metrics that we want to measure
// over a longer period than the system poll interval (e.g. flowRateGPS.. since we want this to be measured for
// as close to a second as possible for accuracy)
struct Telemetry {  
  String stateName;
  long measuredWeightGrams = 0;
  double measuredPressureBars = 0.0;
  double pumpDutyCycle = 0.0;
  double flowRateGPS = 0.0;
  double brewTempC = 0.0;
};

void sendTelemetry();

#endif