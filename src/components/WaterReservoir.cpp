#include "WaterReservoir.h"

WaterReservoirState waterReservoirState;

// send high to turn on solenoid
#define WATER_RESERVOIR_SOLENOID  RX 

// 1 - air , 0 - water
// This sensor draws max 2.5mA so it can be directly powered by 3.3v on
// Argon
#define WATER_RESERVOIR_SENSOR  A1

void waterReservoirInit() {
  
  //Particle.variable("doesWaterNeedFilling",  doesWaterReservoirNeedFilling);

  pinMode(WATER_RESERVOIR_SOLENOID, OUTPUT);

  // water reservoir sensor
  pinMode(WATER_RESERVOIR_SENSOR, INPUT);

}

boolean doesWaterReservoirNeedFilling() {
  
  int needFilling = digitalRead(WATER_RESERVOIR_SENSOR);

  if (needFilling == HIGH) {
    return true;
  } else {
    return false;
  }
}

void turnWaterReservoirSolenoidOn() {
  publishParticleLog("waterReservoirSolenoid", "on");
  digitalWrite(WATER_RESERVOIR_SOLENOID, HIGH);
}

void turnWaterReservoirSolenoidOff() {
  publishParticleLog("waterReservoirSolenoid", "off");
  digitalWrite(WATER_RESERVOIR_SOLENOID, LOW);
}