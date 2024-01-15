#ifndef WATER_PUMP_H
#define WATER_PUMP_H

#include "Common.h"
#include <pid.h>
#include <Adafruit_ADS1X15.h>
#include "Telemetry.h"
#include "Scale.h"

extern double pressure_PID_kP;
extern double pressure_PID_kI;
extern double pressure_PID_kD;

extern double MAX_PUMP_DUTY_CYCLE;
extern double MIN_PUMP_DUTY_CYCLE;

struct WaterPumpState {

  // Only used when Pressure Profiling (e.g. preinfusion, cleaning)
  double targetPressureInBars = -1; 

  double targetFlowRateGPS = -1; 

  // current pressure
  // This is input for the PID
  double measuredPressureInBars;

  // represents how we are currently driving the pump
  // How much of the time we're dispensing out of our total
  // coherence time.. in percentage
  // This is calculated and updated by the PID
  double pumpDutyCycle = -1;

  // The control system for determining when to turn on
  // the pump in order to achieve target pressure
  PID *waterPumpPID;

  double flowRateGPS = 0.0;

  float nextSampleMillis = -1;

  // used to calculate flowRate
  double previousMeasuredWeight = 0.0;
};

extern WaterPumpState waterPumpState;

void waterPumpInit();

// Measures the current pressure in the water pump system
void readPumpState();

void configureWaterPump(int gaggiaState);

void startDispensingWater(boolean turnOnSolenoidValve);

void stopDispensingWater();

// We only update this value at specific intervals
void updateFlowRateMetricIfNecessary();

#endif