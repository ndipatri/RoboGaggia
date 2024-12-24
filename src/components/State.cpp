#include "State.h"

GaggiaState sleepState,
            preheatState,
            cleanOptionsState,
            measureBeansState,
            tareCup2State,
            heatingToBrewState,
            preinfusionState,
            brewingState,
            heatingToDispenseState,
            dispenseHotWaterState,
            doneBrewingState,
            purgeBeforeSteam1, 
            purgeBeforeSteam2, 
            purgeBeforeSteam3, 
            heatingToSteamState, 
            steamingState,
            descaleState,
            coolStartState,
            coolingState,
            coolDoneState,
            groupClean1State,
            groupClean2State,
            groupClean3State,
            backflushInstructions1State,
            backflushInstructions2State,
            backflushInstructions3State,
            backflushCycle1State,
            backflushCycle2State,
            backflushCycleDoneState,
            joiningNetwork,
            ignoringNetwork,
            naState;


// After brewing, we want to message to the user and indicate that 
// they can remove their cup, but we don't want for user input
// before proceeding to heat up for steam... So we just delay a bit in
// case the uesr is watching.. 
int DONE_BREWING_LINGER_TIME_SECONDS = 7;
  
int DONE_PURGE_BEFORE_STEAM_TIME_SECONDS = 3;

int DONE_CLEANING_GROUP_HEAD_SECONDS = 2;

int NUMBER_OF_CLEAN_CYCLES = 20; // (10 on and off based on https://youtu.be/lfJgabTJ-bM?t=38)
int SECONDS_PER_CLEAN_CYCLE = 4; 

// It is possible to manually direct the next station transition
GaggiaState* manualNextGaggiaState;

GaggiaState* currentGaggiaState;

// Using all current state, we derive the next state of the system
// State transitions are documented here:
// https://docs.google.com/drawings/d/1EcaUzklpJn34cYeWsTnApoJhwBhA1Q4EVMr53Kz9T7I/edit?usp=sharing
GaggiaState* getNextGaggiaState() {

  if (manualNextGaggiaState->state != NA) {
    GaggiaState* nextGaggiaState = manualNextGaggiaState;
    manualNextGaggiaState = &naState;

    return nextGaggiaState;
  }

  switch (currentGaggiaState->state) {

    case IGNORING_NETWORK :

      if (WiFi.isOff()) {
        return &preheatState;
      }

      break;

    case JOINING_NETWORK :

      if (networkState.connected) {
        return &preheatState;
      }
      
      if (userInputState.state == SHORT_PRESS || userInputState.state == LONG_PRESS) {
        return &ignoringNetwork;
      }

      break;

    case SLEEP :

      if (userInputState.state == SHORT_PRESS) {
        return &preheatState;
      }
     
      if (userInputState.state == LONG_PRESS) {
        return &preheatState;
      }
      break; 

    case PREHEAT :

      // We have to give the scale long enough to tare proper weight
      if (userInputState.state == SHORT_PRESS) {
        if (heaterState.measuredTemp >= TARGET_BREW_TEMP * 1.50) {
          return &coolStartState;
        } else {
          return &measureBeansState;
        }
      }

      if (userInputState.state == LONG_PRESS) {
        return &cleanOptionsState;
      }
      break;

    case MEASURE_BEANS :

      if (userInputState.state == SHORT_PRESS) {
        return &tareCup2State;
      }
      if (userInputState.state == LONG_PRESS) {
        return &preheatState;
      }
      break;  

    case TARE_CUP_AFTER_MEASURE :

      if (userInputState.state == SHORT_PRESS) {
        return &heatingToBrewState;
      }
      if (userInputState.state == LONG_PRESS) {
        return &preheatState;
      }
      break;

    case HEATING_TO_BREW :

      if (heaterState.measuredTemp >= TARGET_BREW_TEMP) {
        return &preinfusionState;
      }
      if (userInputState.state == LONG_PRESS) {
        return &preheatState;
      }
      break;

    case PREINFUSION :

      if(scaleState.measuredWeight - scaleState.tareWeight > PREINFUSION_WEIGHT_THRESHOLD_GRAMS) {
        return &brewingState;
      }
      if (userInputState.state == LONG_PRESS) {
        return &preheatState;
      }
      break;

    case BREWING :

      if ((scaleState.measuredWeight - scaleState.tareWeight) >= 
            scaleState.targetWeight) {
        return &doneBrewingState;
      }
      if (userInputState.state == LONG_PRESS) {
        return &preheatState;
      }
      break;

    case DONE_BREWING :

      if (userInputState.state == SHORT_PRESS) {
        return &heatingToSteamState;
      }

      if (userInputState.state == LONG_PRESS) {
        return &preheatState;
      }
      break;

    case HEATING_TO_STEAM :

      if (heaterState.measuredTemp >= TARGET_STEAM_TEMP) {
        return &steamingState;
      }
      if (userInputState.state == LONG_PRESS) {
        return &preheatState;
      }
      break;

    case STEAMING :

      if (userInputState.state == SHORT_PRESS) {
        // We want to always clean the group after brewing
        return &groupClean2State;
      }
      if (userInputState.state == LONG_PRESS) {
        // We want to always clean the group after brewing
        return &groupClean2State;
      }
      break;
      
    case DESCALE :

      if (userInputState.state == SHORT_PRESS) {
        return &heatingToDispenseState;
      }
      if (userInputState.state == LONG_PRESS) {
        return &preheatState;
      }
      break;
      
    case HEATING_TO_DISPENSE :

      if (heaterState.measuredTemp >= TARGET_HOT_WATER_DISPENSE_TEMP) {
        return &dispenseHotWaterState;
      }
     
      if (userInputState.state == LONG_PRESS) {
        return &preheatState;
      }
      break; 

    case DISPENSE_HOT_WATER :

      if (userInputState.state == SHORT_PRESS) {
        return &preheatState;
      }
      if (userInputState.state == LONG_PRESS) {
        return &preheatState;
      }
      break;
     
    case COOL_START :

      if (userInputState.state == SHORT_PRESS) {
        return &coolingState;
      }
      if (userInputState.state == LONG_PRESS) {
        return &preheatState;
      }
      break;

    case COOLING :

      if (userInputState.state == SHORT_PRESS) {
        return &coolDoneState;
      }

      if (heaterState.measuredTemp < TARGET_BREW_TEMP) {
        return &coolDoneState;
      }
      if (userInputState.state == LONG_PRESS) {
        return &preheatState;
      }
      break;

    case COOL_DONE :

      if (userInputState.state == SHORT_PRESS) {
        if (heaterState.measuredTemp >= TARGET_BREW_TEMP) {
          return &coolStartState;
        } else {
          return &preheatState;
        }
      }
      if (userInputState.state == LONG_PRESS) {
        return &preheatState;
      }
      break;  
  
    case GROUP_CLEAN_1 :

      if (heaterState.measuredTemp >= TARGET_HOT_WATER_DISPENSE_TEMP) {
        return &groupClean2State;
      }

      if (userInputState.state == LONG_PRESS) {
        return &preheatState;
      }
      break; 
  
    case GROUP_CLEAN_2 :

      if (userInputState.state == SHORT_PRESS) {
          return &groupClean3State;
      }
      if (userInputState.state == LONG_PRESS) {
        return &preheatState;
      }
      break;  
  
  
      case GROUP_CLEAN_3 :

      if ((millis() - currentGaggiaState->stateEnterTimeMillis) > 
             DONE_CLEANING_GROUP_HEAD_SECONDS * 1000) {        
        return &preheatState;
      }     

      if (userInputState.state == LONG_PRESS) {
        return &preheatState;
      }
      break; 
  
    case BACKFLUSH_INSTRUCTION_1 :

      if (userInputState.state == SHORT_PRESS) {
        return &backflushInstructions2State;
      }
     
      if (userInputState.state == LONG_PRESS) {
        return &preheatState;
      }
      break;

    case BACKFLUSH_INSTRUCTION_2 :

      if (userInputState.state == SHORT_PRESS) {
        return &backflushCycle1State;
      }
     
      if (userInputState.state == LONG_PRESS) {
        return &preheatState;
      }
      break;

    case BACKFLUSH_CYCLE_1 :

      if (currentGaggiaState->counter == currentGaggiaState->targetCounter-1) {
        return &backflushInstructions3State;
      }
     
      if (userInputState.state == LONG_PRESS) {
        return &preheatState;
      }
      break;

    case BACKFLUSH_INSTRUCTION_3 :

      if (userInputState.state == SHORT_PRESS) {
        return &backflushCycle2State;
      }
     
      if (userInputState.state == LONG_PRESS) {
        return &preheatState;
      }
      break;

    case BACKFLUSH_CYCLE_2 :

      if (currentGaggiaState->counter == currentGaggiaState->targetCounter-1) {
        return &backflushCycleDoneState;
      }
     
      if (userInputState.state == LONG_PRESS) {
        return &preheatState;
      }
      break;
  
      case BACKFLUSH_CYCLE_DONE :

      if (userInputState.state == SHORT_PRESS) {
        return &preheatState;
      }
     
      if (userInputState.state == LONG_PRESS) {
        return &preheatState;
      }
      break;
  
    case CLEAN_OPTIONS :

      if (userInputState.state == SHORT_PRESS) {
        return &descaleState;
      }
     
      if (userInputState.state == LONG_PRESS) {
        return &backflushInstructions1State;
      }
      break;      

    case PURGE_BEFORE_STEAM_1 :

      if (userInputState.state == SHORT_PRESS) {
        return &purgeBeforeSteam2;
      }
     
      if (userInputState.state == LONG_PRESS) {
        return &preheatState;
      }
      break;   

    case PURGE_BEFORE_STEAM_2 :
     
      if ((millis() - currentGaggiaState->stateEnterTimeMillis) > 
             DONE_PURGE_BEFORE_STEAM_TIME_SECONDS * 1000) {        
        return &purgeBeforeSteam3;
      }     
     
      if (userInputState.state == LONG_PRESS) {
        return &preheatState;
      }
      break;   

    case PURGE_BEFORE_STEAM_3 :

      if (userInputState.state == SHORT_PRESS) {
        return &heatingToSteamState;
      }
     
      if (userInputState.state == LONG_PRESS) {
        return &preheatState;
      }
      break;  
  } 

  // Here we decide to put the system in standby with no heater
  if ((millis() - userInputState.lastUserInteractionTimeMillis) > 
    RETURN_TO_HOME_INACTIVITY_MINUTES * 60 * 1000) {
        return &sleepState;
  }

  return currentGaggiaState;
}  

char* getStateName(int stateEnum) {
   switch (stateEnum) {
    case SLEEP: return "sleep";
    case IGNORING_NETWORK: return "ignoringNetwork";
    case JOINING_NETWORK: return "joiningNetwork";
    case PREHEAT: return "preheat";
    case MEASURE_BEANS: return "measureBeans";
    case TARE_CUP_AFTER_MEASURE: return "tareCupAfterMeasure";
    case HEATING_TO_BREW: return "heating";
    case PREINFUSION: return "preInfusion";
    case BREWING: return "brewing";
    case DONE_BREWING: return "doneBrewing";
    case HEATING_TO_STEAM: return "heatingToSteam";
    case STEAMING: return "steaming";
    case DESCALE: return "descale";
    case GROUP_CLEAN_2: return "cleanGroupReady";
    case GROUP_CLEAN_3: return "cleanGroupDone";
    case COOL_START: return "coolStart";
    case COOLING: return "cooling";
    case COOL_DONE: return "coolDone";
    case BACKFLUSH_INSTRUCTION_1: return "cleanInst1";
    case BACKFLUSH_INSTRUCTION_2: return "cleanInst2";
    case BACKFLUSH_CYCLE_1: return "cleanSoap";
    case BACKFLUSH_INSTRUCTION_3: return "cleanInst3";
    case BACKFLUSH_CYCLE_2: return "cleanRinse";
    case BACKFLUSH_CYCLE_DONE: return "cleanDone";
    case HEATING_TO_DISPENSE: return "heatingToDispense";
    case DISPENSE_HOT_WATER: return "dispenseHotWater";
    case CLEAN_OPTIONS: return "cleanOptions";
    case NA: return "na";
  }
  
  return "na";
}

void processIncomingGaggiaState(GaggiaState *nextGaggiaState) {
  
  if (nextGaggiaState->waterThroughGroupHead || nextGaggiaState->waterThroughWand) {
    publishParticleLog("dispense", "Launching PID");

    configureWaterPump(nextGaggiaState->state);
  }

  if (nextGaggiaState->brewHeaterOn) {
    configureBrewHeater();
  }

  if (nextGaggiaState->steamHeaterOn) {
    configureSteamHeater();
  }

  if (nextGaggiaState->hotWaterDispenseHeaterOn) {
    configureHotWaterDispenseHeater();
  }

  if (nextGaggiaState->state == PREHEAT) {
      scaleState.tareWeight = 0.0;
  }
}

void processCurrentGaggiaState() { 

  long nowTimeMillis = millis();

  // Process Heaters
  //
  // Even though a heater may be 'on' during this state, the control system
  // for the heater turns it off and on intermittently in an attempt to regulate
  // the temperature around the target temp.
  if (currentGaggiaState->brewHeaterOn) {
    readSteamHeaterState();  

    if (shouldTurnOnHeater()) {
      turnHeaterOn();
    } else {
      turnHeaterOff();
    } 
  }
  if (currentGaggiaState->steamHeaterOn) {
    readSteamHeaterState(); 

    if (shouldTurnOnHeater()) {
      turnHeaterOn();
    } else {
      turnHeaterOff();
    } 
  }
  if (currentGaggiaState->hotWaterDispenseHeaterOn) {
    readSteamHeaterState(); 

    if (shouldTurnOnHeater()) {
      turnHeaterOn();
    } else {
      turnHeaterOff();
    } 
  }
  if (!currentGaggiaState->brewHeaterOn && !currentGaggiaState->steamHeaterOn && !currentGaggiaState->hotWaterDispenseHeaterOn) {
      turnHeaterOff();
  }

  if (currentGaggiaState->fillingReservoir) {

    if (doesWaterReservoirNeedFilling()) {
      waterReservoirState.isSolenoidOn = true;
      turnWaterReservoirSolenoidOn();
    } else {
      waterReservoirState.isSolenoidOn = false;
      turnWaterReservoirSolenoidOff();
    }

  } else {
    turnWaterReservoirSolenoidOff();
  }

  if (currentGaggiaState->measureTemp) {
    readSteamHeaterState();
  }

  if (currentGaggiaState->waterThroughGroupHead || currentGaggiaState->waterThroughWand) {
    if (currentGaggiaState->state == BACKFLUSH_CYCLE_1 || currentGaggiaState->state == BACKFLUSH_CYCLE_2) {
      // whether or not we dispense water depends on where we are in the clean cycle...      

      currentGaggiaState->targetCounter = NUMBER_OF_CLEAN_CYCLES;

      if (currentGaggiaState->counter % 2 == 0) { // 0, and even counts 0, 2, 4...
        // we dispense water.. this fills portafilter with pressured hot water...
        startDispensingWater(true); // going through grouphead
      } else { // odd counts 1,3,5...
        // we stop dispensing water .. this backfushes hot water through group head...
        stopDispensingWater();
      }
  
      // when we first enter state, stopTimeMillis is -1, so this condition is true
      if (nowTimeMillis > currentGaggiaState->stopTimeMillis) {
                
        // when we first enter this state, counter is -1, so it gets bumped to 0.
        currentGaggiaState->counter = currentGaggiaState->counter + 1; 
        
        // start new cleaning cycle
        double timeMultiplier = 1;
        if (currentGaggiaState->counter % 2 != 0) {
          // odd, or off, cycles should be shorter
          timeMultiplier = .2;
        }
        currentGaggiaState->stopTimeMillis = nowTimeMillis + SECONDS_PER_CLEAN_CYCLE*timeMultiplier*1000;
      }
    } else {
      // normal dispense state
      startDispensingWater(currentGaggiaState->waterThroughGroupHead);
    }
  } else {
    stopDispensingWater();
  }

  if (currentGaggiaState->state == IGNORING_NETWORK) {

    // we do this just incase we spipped network AFTER
    // we achieved a connection.
    networkState.connected = false;
    Particle.disconnect();
    WiFi.off();
  }

  if (currentGaggiaState->state == JOINING_NETWORK) {

    if (WiFi.connecting()) {
      return;
    }

    if (!Particle.connected()) {
      Particle.connect(); 
    } else {
      networkState.connected = true;
    }
  }

  if (currentGaggiaState->state == BREWING || currentGaggiaState->state == PREINFUSION) {
    updateFlowRateMetricIfNecessary();
  }

  if (currentGaggiaState->state == SLEEP) {
    if (networkState.connected) {
      // recall the system returns to hello after 15 minutes of inactivity.
      sendTelemetryIfNecessary(true);
    }
  }
}

void processOutgoingGaggiaState() {

  // Good time to pick up any MQTT erors...
  // this is our 'wait for incoming subscription packets' busy subloop
  // try to spend your time here
  //
  // NJD - this seems to take too long,. ignoring for now...
  // Adafruit_MQTT_Subscribe *subscription;
  // while ((subscription = mqtt.readSubscription(3000))) {
  //   if(subscription == &errors) {
  //     publishParticleLog("mqtt", "Error: (" + String((char *)errors.lastread) + ").");
  //   } else if(subscription == &throttle) {
  //     publishParticleLog("mqtt", "Throttle: (" + String((char *)throttle.lastread) + ").");
  //   }
  // }

  // Process Tare Scale
  
    // it is assumed the user has been instructed to clear the scale during these states.
    if (currentGaggiaState->state == JOINING_NETWORK || 
        currentGaggiaState->state == IGNORING_NETWORK || 
        currentGaggiaState->state == SLEEP) {
      zeroScale();
    }
  
  // WARNING! This has to happen before we tare the scale for PREHEAT
  if (currentGaggiaState->state == PREHEAT) {
    // we know the scale has just the cup on it with a known weight.
    calibrateScale();
    delay(50);
    readScaleState();
  }

  // WARNING! This has to happen after we calibrate for PREHEAT 
  if (currentGaggiaState->tareScale) {
    scaleState.tareWeight = scaleState.measuredWeight;
  }

  // Process Record Weight 
  if (currentGaggiaState->recordWeight) {
    
    int weightToBeanRatio = loadSettings().weightToBeanRatio;

    scaleState.targetWeight = 
      (scaleState.measuredWeight - scaleState.tareWeight)*weightToBeanRatio; 
  }

  // This gives our current state one last chance to log any telemetry.
  if (currentGaggiaState->state == BREWING) {
    updateFlowRateMetricIfNecessary();

    increaseBrewCount();
  }

  if (currentGaggiaState->state == BACKFLUSH_CYCLE_DONE) {
    clearBackflushBrewCount();
  }

  // Things we always reset when leaving a state...
  currentGaggiaState->stopTimeMillis = -1;
  currentGaggiaState->counter = -1;
}

String readCurrentState() {
  return String(currentGaggiaState->state);
}

int setCoolingState(String _) {
  manualNextGaggiaState = &coolingState;
  return 1;
}

int setSteamingState(String _) {
  manualNextGaggiaState = &steamingState;
  return 1;
}

int setDispenseHotWater(String _) {
  manualNextGaggiaState = &dispenseHotWaterState;
  return 1;
}

void stateInit() {

  Particle.variable("currentState", readCurrentState);

  Particle.function("setDispenseHotWater", setDispenseHotWater);
  Particle.function("setSteamingState", setSteamingState);

 // Define all possible states of RoboGaggia

  sleepState.state = SLEEP;

  joiningNetwork.state = JOINING_NETWORK;

  ignoringNetwork.state = IGNORING_NETWORK;
  
  // entry point when first power or if leaving sleep
  preheatState.state = PREHEAT;

  // We need to measure temp when leaving to know if we need to
  // do cooling phase first...
  preheatState.measureTemp = true;

  // we don't want to heat here in case the unit was turned on and
  // person walked away for a few days ...
  preheatState.brewHeaterOn = true; 

  // we tare here so the weight of the scale itself and the cup aren't shown
  // when we are measuring things...
  preheatState.tareScale = true; 

  measureBeansState.state = MEASURE_BEANS; 
  measureBeansState.recordWeight = true; 
  measureBeansState.brewHeaterOn = true; 
  measureBeansState.fillingReservoir = true;

  tareCup2State.state = TARE_CUP_AFTER_MEASURE; 
  tareCup2State.tareScale = true; 
  tareCup2State.brewHeaterOn = true; 

  heatingToBrewState.state = HEATING_TO_BREW; 
  heatingToBrewState.brewHeaterOn = true; 

  preinfusionState.state = PREINFUSION; 
  preinfusionState.waterThroughGroupHead = true; 
  preinfusionState.brewHeaterOn = true; 

  brewingState.state = BREWING; 
  brewingState.brewHeaterOn = true; 
  brewingState.waterThroughGroupHead = true; 

  doneBrewingState.state = DONE_BREWING; 
  doneBrewingState.brewHeaterOn = true; 
  doneBrewingState.fillingReservoir = true;

  heatingToSteamState.state = HEATING_TO_STEAM; 
  heatingToSteamState.steamHeaterOn = true; 

  steamingState.state = STEAMING; 
  steamingState.steamHeaterOn = true; 

  descaleState.state = DESCALE; 
  descaleState.hotWaterDispenseHeaterOn = true; 

  heatingToDispenseState.state = HEATING_TO_DISPENSE; 
  // NJD TODO - This seems strange but we need to keep the heater goign in order to keep 
  // updated telemetry publishing.. I should fix this so telemtry goes out regardless
  heatingToDispenseState.hotWaterDispenseHeaterOn = true; 

  dispenseHotWaterState.state = DISPENSE_HOT_WATER; 
  dispenseHotWaterState.hotWaterDispenseHeaterOn = true; 
  dispenseHotWaterState.waterThroughWand = true; 

  purgeBeforeSteam1.state = PURGE_BEFORE_STEAM_1; 
  purgeBeforeSteam1.brewHeaterOn = true; 

  purgeBeforeSteam2.state = PURGE_BEFORE_STEAM_2; 
  purgeBeforeSteam2.brewHeaterOn = true; 
  purgeBeforeSteam2.waterThroughGroupHead = true; 

  purgeBeforeSteam3.state = PURGE_BEFORE_STEAM_3; 
  purgeBeforeSteam3.steamHeaterOn = true; 

  coolStartState.state = COOL_START; 
  coolStartState.fillingReservoir = true;

  coolingState.state = COOLING; 
  coolingState.waterThroughGroupHead = true; 
  coolingState.measureTemp = true;

  coolDoneState.state = COOL_DONE;

  groupClean1State.state = GROUP_CLEAN_1; 
  groupClean1State.hotWaterDispenseHeaterOn = true; 

  groupClean2State.state = GROUP_CLEAN_2; 
  groupClean2State.hotWaterDispenseHeaterOn = true; 

  groupClean3State.state = GROUP_CLEAN_3;
  groupClean3State.hotWaterDispenseHeaterOn = true; 
  groupClean3State.waterThroughGroupHead = true; 

  backflushInstructions1State.state = BACKFLUSH_INSTRUCTION_1;

  backflushInstructions2State.state = BACKFLUSH_INSTRUCTION_2;

  backflushInstructions3State.state = BACKFLUSH_INSTRUCTION_3;

  backflushCycle1State.state = BACKFLUSH_CYCLE_1;
  backflushCycle1State.brewHeaterOn = true; 
  backflushCycle1State.waterThroughGroupHead = true; 

  backflushCycle2State.state = BACKFLUSH_CYCLE_2;
  backflushCycle2State.brewHeaterOn = true; 
  backflushCycle2State.waterThroughGroupHead = true; 
  // trying to fill reservoir during clean seemed to cause problems..
  // not sure why

  backflushCycleDoneState.state = BACKFLUSH_CYCLE_DONE;

  cleanOptionsState.state = CLEAN_OPTIONS; 
  cleanOptionsState.fillingReservoir = true;
  // NJD TODO - This seems strange but we need to keep the heater goign in order to keep 
  // updated telemetry publishing.. I should fix this so telemtry goes out regardless
  cleanOptionsState.hotWaterDispenseHeaterOn = true; 

  naState.state = NA;

  currentGaggiaState = &joiningNetwork;

  manualNextGaggiaState = &naState;
}
