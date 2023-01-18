#ifndef CORE_H
#define CORE_H

#include <Arduino.h>
#include "Secrets.h"

enum GaggiaStateEnum {
  HELLO , 
  FEATURES , 
  TARE_CUP_BEFORE_MEASURE , 
  MEASURE_BEANS ,  
  TARE_CUP_AFTER_MEASURE , 
  HEATING_TO_BREW , 
  PREINFUSION , 
  BREWING , 
  DONE_BREWING , 
  HEATING_TO_STEAM , 
  STEAMING , 
  COOL_START , 
  COOLING , 
  COOL_DONE , 
  CLEAN_INSTRUCTION_1 , 
  CLEAN_INSTRUCTION_2 , 
  CLEAN_SOAP , 
  CLEAN_INSTRUCTION_3 , 
  CLEAN_RINSE , 
  CLEAN_DONE , 
  HEATING_TO_DISPENSE , 
  DISPENSE_HOT_WATER , 
  IGNORING_NETWORK , 
  JOINING_NETWORK , 
  NA // indicates developer is NOT explicitly setting a test state through web interface
};

struct GaggiaState {
   int state;   
   char *display1 = "";
   char *display2 = "";
   char *display3 = "";
   char *display4 = "";
   
   // NJD TODO - do we need this anymore?
   boolean waterReservoirSolenoidOn = false;
   
   // ************
   // WARNING: this cannot be done at the same time (or even in an adjacent state)
   // to dispensing water.
   // ************
   boolean  fillingReservoir = false;

   boolean  brewHeaterOn = false;
   boolean  steamHeaterOn = false;
   boolean  hotWaterDispenseHeaterOn = false;
   boolean  measureTemp = false;
   boolean  tareScale = false;
   boolean  waterThroughGroupHead = false;
   boolean  waterThroughWand = false;
   boolean  recordWeight = false;

   float stateEnterTimeMillis = -1;
   
   // an arbitrary timer that can be used to schedule
   // future events.
   float stopTimeMillis = -1;

   // An arbitrary counter that can be used
   int counter = -1;
   int targetCounter = -1;
};

void commonInit();

void publishParticleLogNow(String group, String message);

void publishParticleLog(String group, String message);

int turnOnTestMode(String _na);

int turnOffTestMode(String _na);

int enterDFUMode(String _na);

extern boolean isInTestMode;

#endif