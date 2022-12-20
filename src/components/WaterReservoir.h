#ifndef WATER_RESERVOIR_H
#define WATER_RESERVOIR_H

#include "Common.h"

struct WaterReservoirState {
  boolean isSolenoidOn = false;
};

extern WaterReservoirState waterReservoirState;

void waterReservoirInit();

boolean doesWaterReservoirNeedFilling();

void turnWaterReservoirSolenoidOn();

void turnWaterReservoirSolenoidOff();

#endif
