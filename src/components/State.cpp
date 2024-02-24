#include "State.h"

#include <Qwiic_Scale_NAU7802_Arduino_Library.h>

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

String updateDisplayLine(String message, 
                         int lineNumber,
                         String previousLineDisplayed) {

  display.setCursor(0,lineNumber-1);
  String lineToDisplay;
  
  // Mostly, the passed message IS what will be displayed, but there
  // are 'escape' values that, if present, need to be replaced with
  // state information....
  boolean lineHandled = false; 
  
  if (!lineHandled && message.indexOf("{helloMessage}") != -1) {
    lineHandled = true;

    if (shouldBackflush()) {
      lineToDisplay = "Hi. Backflush Time!";
    } else {
      lineToDisplay = String("Hi. (");
      lineToDisplay.concat(shotsUntilBackflush());
      lineToDisplay.concat(",");
      lineToDisplay.concat(readTotalBrewCount());
      lineToDisplay.concat(") shots");
    }
  }

  if (!lineHandled && message.indexOf("{adjustedWeight}/{targetBeanWeight}") != -1) {
    lineHandled = true;

    lineToDisplay = renderDoubleValue(scaleState.measuredWeight - scaleState.tareWeight);
    lineToDisplay.concat("/");
    lineToDisplay.concat(renderDoubleValue(TARGET_BEAN_WEIGHT));
    lineToDisplay.concat(" g");
  }
  
  if (!lineHandled && message.indexOf("{measuredBrewTemp}/{targetBrewTemp}") != -1) {
    lineHandled = true;

    lineToDisplay = String((long)heaterState.measuredTemp);
    lineToDisplay.concat("/");
    lineToDisplay.concat((long)TARGET_BREW_TEMP);
    lineToDisplay.concat(" degrees C");
  }

  if (!lineHandled && message.indexOf("{adjustedWeight}/{targetBrewWeight}") != -1) {
    lineHandled = true;

    lineToDisplay = renderDoubleValue(scaleState.measuredWeight - scaleState.tareWeight);
    lineToDisplay.concat("/");
    lineToDisplay.concat(renderDoubleValue(scaleState.targetWeight));
    lineToDisplay.concat(" g");
  }

  if (!lineHandled && message.indexOf("{measuredSteamTemp}/{targetSteamTemp}") != -1) {
    lineHandled = true;

    lineToDisplay = String((long)heaterState.measuredTemp);
    lineToDisplay.concat("/");
    lineToDisplay.concat((long)TARGET_STEAM_TEMP);
    lineToDisplay.concat(" degrees C");
  }

  if (!lineHandled && message.indexOf("{adjustedWeight}") != -1) {
    lineHandled = true;

    lineToDisplay = renderDoubleValue(scaleState.measuredWeight - scaleState.tareWeight);
    lineToDisplay.concat(" g");
  }  

  if (!lineHandled && message.indexOf("{flowRate}") != -1) {
    lineHandled = true;

    lineToDisplay = String((long)waterPumpState.flowRateGPS);
    lineToDisplay.concat(" g/30sec");
  }  

  if (!lineHandled && message.indexOf("{measuredBars}/{targetBars}") != -1) {
    lineHandled = true;

    lineToDisplay = String((long)waterPumpState.measuredPressureInBars);
    lineToDisplay.concat("/");
    lineToDisplay.concat((long)waterPumpState.targetPressureInBars);
    lineToDisplay.concat(" bars");
  } 

  if (!lineHandled && message.indexOf("{currentPass}/{targetPass}") != -1) {
    lineHandled = true;

    // One count consists of both the clean and rest... so we only have
    // targetCounter/2 total clean passes.
    lineToDisplay = String((long)(currentGaggiaState->counter/2)+1);
    lineToDisplay.concat("/");
    lineToDisplay.concat((long)(currentGaggiaState->targetCounter)/2);
    lineToDisplay.concat(" pass");
  } 

  if (!lineHandled && message.indexOf("{pumpDutyCycle}/{maxDutyCycle}") != -1) {
    lineHandled = true;

    lineToDisplay = String((long)waterPumpState.pumpDutyCycle);
    lineToDisplay.concat("/");
    lineToDisplay.concat((long)MAX_PUMP_DUTY_CYCLE);
    lineToDisplay.concat(" %");
  } 

  if (!lineHandled && message.indexOf("{measuredSteamTemp}/{targetHotWaterDispenseTemp}") != -1) {
    lineHandled = true;

    lineToDisplay = String((long)heaterState.measuredTemp);
    lineToDisplay.concat("/");
    lineToDisplay.concat((long)TARGET_HOT_WATER_DISPENSE_TEMP);
    lineToDisplay.concat(" degrees C");
  } 

  if (!lineHandled && message.indexOf("{elapsedTimeSeconds}") != -1) {
    lineHandled = true;

    lineToDisplay = String((long)(millis() - currentGaggiaState->stateEnterTimeMillis)/1000);
    lineToDisplay.concat(" seconds");
  }  

  if (!lineHandled && message.indexOf("{liveExtractionTimes}") != -1) {
    lineHandled = true;

    double preinfusionTimeSeconds = (preinfusionState.stateExitTimeMillis - preinfusionState.stateEnterTimeMillis)/1000.0;
    lineToDisplay = String((long)preinfusionTimeSeconds);
    lineToDisplay.concat("/");
    lineToDisplay.concat((long)(millis() - currentGaggiaState->stateEnterTimeMillis)/1000);
    lineToDisplay.concat(" seconds");
  } 

  if (!lineHandled && message.indexOf("{extractionTimes}") != -1) {
    lineHandled = true;

    double preinfusionTimeSeconds = (preinfusionState.stateExitTimeMillis - preinfusionState.stateEnterTimeMillis)/1000.0;
    double brewTimeSeconds = (brewingState.stateExitTimeMillis - brewingState.stateEnterTimeMillis)/1000.0;
    lineToDisplay = String((long)preinfusionTimeSeconds);
    lineToDisplay.concat("/");
    lineToDisplay.concat((long)brewTimeSeconds);
    lineToDisplay.concat(" seconds");
  } 
                  
  if (!lineHandled) {
    lineToDisplay = String(message);
  }

  // For the display, you must display a length of 20
  lineToDisplay.concat(String("                    ").substring(0, 20 - lineToDisplay.length()));

  if (lineNumber == 1) {
    // inject 'heating' indicator
    if (isHeaterOn()) {
      lineToDisplay.setCharAt(19, '*');
    }
  }

  // This prevents flashing
  if (lineToDisplay != previousLineDisplayed) {
    display.print(lineToDisplay);
  }

  // Need to return this so we can check for changes 
  // next time
  return lineToDisplay;
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
    publishParticleLog("dispense", "Launching Pressure PID");

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

  // 
  // Process Display
  //
  displayState.display1 = updateDisplayLine(currentGaggiaState->display1, 
                                             1, 
                                             displayState.display1);

  displayState.display2 = updateDisplayLine(currentGaggiaState->display2, 
                                             2, 
                                             displayState.display2);

  displayState.display3 = updateDisplayLine(currentGaggiaState->display3, 
                                             3, 
                                             displayState.display3);

  displayState.display4 = updateDisplayLine(currentGaggiaState->display4, 
                                             4, 
                                             displayState.display4);

  long nowTimeMillis = millis();

  // Process Heaters
  //
  // Even though a heater may be 'on' during this state, the control system
  // for the heater turns it off and on intermittently in an attempt to regulate
  // the temperature around the target temp.
  if (currentGaggiaState->brewHeaterOn) {
    readBrewHeaterState();  

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
    readBrewHeaterState();
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

  #ifdef AIO_USERNAME
    if (currentGaggiaState->state == SLEEP) {
      // we disconnect from MQTT broker for telemetry...
      if (networkState.connected) {
        // recall the system returns to hello after 15 minutes of inactivity.
        sendTelemetryIfNecessary(true);
        MQTTDisconnect();
      }
    } else {
      // we want telemetry to be available for all non-rest states...
      if (networkState.connected) {
        MQTTConnect();
      }
    }
  #endif
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
  if (currentGaggiaState->tareScale) {
    scaleState.tareWeight = scaleState.measuredWeight;
  }

  // Process Record Weight 
  if (currentGaggiaState->recordWeight) {
    scaleState.targetWeight = 
      (scaleState.measuredWeight - scaleState.tareWeight)*BREW_WEIGHT_TO_BEAN_RATIO; 
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

int setHeatingState(String _) {
  
  // by jumping to this state, we are bypassing earlier states.. so we need to
  // fake any data that should have been collected.
  scaleState.tareWeight = 320;
  scaleState.targetWeight = 100;

  manualNextGaggiaState = &heatingToBrewState;
  return 1;
}

int setCoolingState(String _) {
  manualNextGaggiaState = &coolingState;
  return 1;
}

int setSteamingState(String _) {
  manualNextGaggiaState = &steamingState;
  return 1;
}

int setBrewingState(String _) {
  
  // by jumping to this state, we are bypassing earlier states.. so we need to
  // fake any data that should have been collected.
  scaleState.tareWeight = 320;
  scaleState.targetWeight = 100;

  manualNextGaggiaState = &brewingState;
  return 1;
}

int setPreInfusionState(String _) {
  
  // by jumping to this state, we are bypassing earlier states.. so we need to
  // fake any data that should have been collected.
  scaleState.tareWeight = 320;
  scaleState.targetWeight = 100;

  manualNextGaggiaState = &preinfusionState;
  return 1;
}

int setDispenseHotWater(String _) {
  manualNextGaggiaState = &dispenseHotWaterState;
  return 1;
}


void stateInit() {

  Particle.variable("currentState", readCurrentState);

  Particle.function("setDispenseHotWater", setDispenseHotWater);
  Particle.function("setPreinfusionState", setPreInfusionState);
  Particle.function("setHeatingState", setHeatingState);
  Particle.function("setBrewingState", setBrewingState);
  Particle.function("setSteamingState", setSteamingState);

 // Define all possible states of RoboGaggia

  sleepState.state = SLEEP;
  sleepState.display1 =            "Hi (yawn).          ";
  sleepState.display2 =            "I'm sleeping.       ";
  sleepState.display3 =            "                    ";
  sleepState.display4 =            "Click to wake me up.";
  
  joiningNetwork.state = JOINING_NETWORK;
  joiningNetwork.display1 =            "Joining network ... ";
  joiningNetwork.display2 =            "                    ";
  joiningNetwork.display3 =            "                    ";
  joiningNetwork.display4 =            "Click to Skip       ";

  ignoringNetwork.state = IGNORING_NETWORK;
  ignoringNetwork.display1 =            "Ignoring Network    ";
  ignoringNetwork.display2 =            "                    ";
  ignoringNetwork.display3 =            "                    ";
  ignoringNetwork.display4 =            "Please wait ...     ";
  
  // entry point when first power or if leaving sleep
  preheatState.state = PREHEAT;
  preheatState.display1 =            "{helloMessage}";
  preheatState.display2 =            "Empty cup on scale. ";
  preheatState.display3 =            "Click to Brew,      ";
  preheatState.display4 =            "Hold for Clean      ";

  // We need to measure temp when leaving to know if we need to
  // do cooling phase first...
  preheatState.measureTemp = true;

  // we don't want to heat here in case the unit was turned on and
  // person walked away for a few days ...
  preheatState.brewHeaterOn = true; 

  // we tare here so the weight of the scale itself isn't shown
  // when we are measuring things...
  preheatState.tareScale = true; 

  measureBeansState.state = MEASURE_BEANS; 
  measureBeansState.display1 =     "Add beans to cup.   ";
  measureBeansState.display2 =     "{adjustedWeight}/{targetBeanWeight}";
  measureBeansState.display3 =     "                    ";
  measureBeansState.display4 =     "Click when Ready    ";
  measureBeansState.recordWeight = true; 
  measureBeansState.brewHeaterOn = true; 
  measureBeansState.fillingReservoir = true;

  tareCup2State.state = TARE_CUP_AFTER_MEASURE; 
  tareCup2State.display1 =         "Grind & load beans, ";
  tareCup2State.display2 =         "Place empty cup     ";
  tareCup2State.display3 =         "back on tray.       ";
  tareCup2State.display4 =         "Click when Ready    ";
  tareCup2State.tareScale = true; 
  tareCup2State.brewHeaterOn = true; 

  heatingToBrewState.state = HEATING_TO_BREW; 
  heatingToBrewState.display1 =    "Heating to brew.    ";
  heatingToBrewState.display2 =    "{measuredBrewTemp}/{targetBrewTemp}";
  heatingToBrewState.display3 =    "                    ";
  heatingToBrewState.display4 =    "Please wait ...     ";
  heatingToBrewState.brewHeaterOn = true; 

  preinfusionState.state = PREINFUSION; 
  preinfusionState.display1 =      "Infusing coffee.    ";
  preinfusionState.display2 =      "{measuredBars}/{targetBars}";
  preinfusionState.display3 =      "                    ";
  preinfusionState.display4 =      "{elapsedTimeSeconds}";
  preinfusionState.waterThroughGroupHead = true; 
  preinfusionState.brewHeaterOn = true; 

  brewingState.state = BREWING; 
  brewingState.display1 =          "Brewing.            ";
  brewingState.display2 =          "{measuredBars}/{targetBars}";
  brewingState.display3 =          "{adjustedWeight}/{targetBrewWeight}";
  brewingState.display4 =          "{liveExtractionTimes}";
  brewingState.brewHeaterOn = true; 
  brewingState.waterThroughGroupHead = true; 

  doneBrewingState.state = DONE_BREWING; 
  doneBrewingState.display1 =     "Done brewing.       ";
  doneBrewingState.display2 =     "{extractionTimes}   ";
  doneBrewingState.display3 =     "Remove cup.         ";
  doneBrewingState.display4 =     "Click to steam ...  ";
  doneBrewingState.brewHeaterOn = true; 
  doneBrewingState.fillingReservoir = true;

  heatingToSteamState.state = HEATING_TO_STEAM; 
  heatingToSteamState.display1 =   "Heating to steam.   ";
  heatingToSteamState.display2 =   "{measuredSteamTemp}/{targetSteamTemp}";
  heatingToSteamState.display3 =   "                    ";
  heatingToSteamState.display4 =   "Please wait ...     ";
  heatingToSteamState.steamHeaterOn = true; 

  steamingState.state = STEAMING; 
  steamingState.display1 =         "Operate steam wand. ";
  steamingState.display2 =         "{measuredSteamTemp}/{targetSteamTemp}";
  steamingState.display3 =         "                    ";
  steamingState.display4 =         "Click when Done     ";
  steamingState.steamHeaterOn = true; 

  descaleState.state = DESCALE; 
  descaleState.display1 =         "Fill the reservoir  ";
  descaleState.display2 =         "with descale liquid ";
  descaleState.display3 =         "or water and        ";
  descaleState.display4 =         "Click when Ready    ";
  descaleState.hotWaterDispenseHeaterOn = true; 

  heatingToDispenseState.state = HEATING_TO_DISPENSE; 
  heatingToDispenseState.display1 =   "Heating to dispense ";
  heatingToDispenseState.display2 =   "          hot water.";
  heatingToDispenseState.display3 =   "{measuredSteamTemp}/{targetHotWaterDispenseTemp}";
  heatingToDispenseState.display4 =   "Please wait ...     ";
  // NJD TODO - This seems strange but we need to keep the heater goign in order to keep 
  // updated telemetry publishing.. I should fix this so telemtry goes out regardless
  heatingToDispenseState.hotWaterDispenseHeaterOn = true; 

  dispenseHotWaterState.state = DISPENSE_HOT_WATER; 
  dispenseHotWaterState.display1 =    "Use steam wand to  ";
  dispenseHotWaterState.display2 =    "dispense hot water. ";
  dispenseHotWaterState.display3 =    "                    ";
  dispenseHotWaterState.display4 =    "Click when Done     ";
  dispenseHotWaterState.hotWaterDispenseHeaterOn = true; 
  dispenseHotWaterState.waterThroughWand = true; 

  purgeBeforeSteam1.state = PURGE_BEFORE_STEAM_1; 
  purgeBeforeSteam1.display1 =   "Boiler needs water. ";
  purgeBeforeSteam1.display2 =   "Remove scale and    ";
  purgeBeforeSteam1.display3 =   "place cup on tray.  ";
  purgeBeforeSteam1.display4 =   "Click when Ready    ";
  purgeBeforeSteam1.brewHeaterOn = true; 

  purgeBeforeSteam2.state = PURGE_BEFORE_STEAM_2; 
  purgeBeforeSteam2.display1 =   "Pumping water into  ";
  purgeBeforeSteam2.display2 =   "boiler.             ";
  purgeBeforeSteam2.display3 =   "                    ";
  purgeBeforeSteam2.display4 =   "Please wait ...     ";
  purgeBeforeSteam2.brewHeaterOn = true; 
  purgeBeforeSteam2.waterThroughGroupHead = true; 

  purgeBeforeSteam3.state = PURGE_BEFORE_STEAM_3; 
  purgeBeforeSteam3.display1 =   "Boiler is now ready ";
  purgeBeforeSteam3.display2 =   "to steam.           ";
  purgeBeforeSteam3.display3 =   "                    ";
  purgeBeforeSteam3.display4 =   "Click when Ready    ";
  purgeBeforeSteam3.steamHeaterOn = true; 

  coolStartState.state = COOL_START; 
  coolStartState.display1 =        "Too hot to brew,    ";
  coolStartState.display2 =        "Need to run water.  ";
  coolStartState.display3 =        "Place cup on tray.  ";
  coolStartState.display4 =        "Click when Ready    ";
  coolStartState.fillingReservoir = true;

  coolingState.state = COOLING; 
  coolingState.display1 =          "Dispensing to cool  ";
  coolingState.display2 =          "{measuredBrewTemp}/{targetBrewTemp}";
  coolingState.display3 =          "                    ";
  coolingState.display4 =          "Click to Stop       ";
  coolingState.waterThroughGroupHead = true; 
  coolingState.measureTemp = true;

  coolDoneState.state = COOL_DONE;
  coolDoneState.display1 =         "Discard water.      ";
  coolDoneState.display2 =         "                    ";
  coolDoneState.display3 =         "                    ";
  coolDoneState.display4 =         "Click when Ready    ";

  groupClean1State.state = GROUP_CLEAN_1; 
  groupClean1State.display1 =   "Heating to Clean    ";
  groupClean1State.display2 =   "Group               ";
  groupClean1State.display3 =   "{measuredSteamTemp}/{targetHotWaterDispenseTemp}";
  groupClean1State.display4 =   "Please wait ...     ";
  groupClean1State.hotWaterDispenseHeaterOn = true; 

  groupClean2State.state = GROUP_CLEAN_2; 
  groupClean2State.display1 =   "Ready to Clean    ";
  groupClean2State.display2 =   "Remove portafilter";
  groupClean2State.display3 =   "and place towel   ";
  groupClean2State.display4 =   "Click when Ready  ";
  groupClean2State.hotWaterDispenseHeaterOn = true; 

  groupClean3State.state = GROUP_CLEAN_3;
  groupClean3State.display1 =   "Cleaning Group Head ";
  groupClean3State.display2 =   "                    ";
  groupClean3State.display3 =   "                    ";
  groupClean3State.display4 =   "Please wait ...     ";
  groupClean3State.hotWaterDispenseHeaterOn = true; 
  groupClean3State.waterThroughGroupHead = true; 

  backflushInstructions1State.state = BACKFLUSH_INSTRUCTION_1;
  backflushInstructions1State.display1 =            "Load backflush      ";
  backflushInstructions1State.display2 =            "portafilter with    ";
  backflushInstructions1State.display3 =            "3g of Cafiza.       ";
  backflushInstructions1State.display4 =            "Click when Ready    ";

  backflushInstructions2State.state = BACKFLUSH_INSTRUCTION_2;
  backflushInstructions2State.display1 =            "Remove scale        ";
  backflushInstructions2State.display2 =            "from the drain pan. ";
  backflushInstructions2State.display3 =            "                    ";
  backflushInstructions2State.display4 =            "Click when Ready    ";

  backflushInstructions3State.state = BACKFLUSH_INSTRUCTION_3;
  backflushInstructions3State.display1 =            "Clean out backflush ";
  backflushInstructions3State.display2 =            "portafilter.        ";
  backflushInstructions3State.display3 =            "                    ";
  backflushInstructions3State.display4 =            "Click when Ready    ";

  backflushCycle1State.state = BACKFLUSH_CYCLE_1;
  backflushCycle1State.display1 =            "Backflushing        ";
  backflushCycle1State.display2 =            "with cleaner.       ";
  backflushCycle1State.display3 =            "{measuredBars}/{targetBars}";
  backflushCycle1State.display4 =            "{currentPass}/{targetPass}";
  backflushCycle1State.brewHeaterOn = true; 
  backflushCycle1State.waterThroughGroupHead = true; 

  backflushCycle2State.state = BACKFLUSH_CYCLE_2;
  backflushCycle2State.display1 =            "Backflushing        ";
  backflushCycle2State.display2 =            "with water.         ";
  backflushCycle2State.display3 =            "{measuredBars}/{targetBars}";
  backflushCycle2State.display4 =            "{currentPass}/{targetPass}";
  backflushCycle2State.brewHeaterOn = true; 
  backflushCycle2State.waterThroughGroupHead = true; 
  // trying to fill reservoir during clean seemed to cause problems..
  // not sure why

  backflushCycleDoneState.state = BACKFLUSH_CYCLE_DONE;
  backflushCycleDoneState.display1 =            "Replace normal      ";
  backflushCycleDoneState.display2 =            "portafilter.        ";
  backflushCycleDoneState.display3 =            "Return scale.       ";
  backflushCycleDoneState.display4 =            "Click when Done     ";

  cleanOptionsState.state = CLEAN_OPTIONS; 
  cleanOptionsState.display1 =            "Select Clean Option ";
  cleanOptionsState.display2 =            "                    ";
  cleanOptionsState.display3 =            "Click for Descale,  ";
  cleanOptionsState.display4 =            "Hold for Backflush  ";
  cleanOptionsState.fillingReservoir = true;
  // NJD TODO - This seems strange but we need to keep the heater goign in order to keep 
  // updated telemetry publishing.. I should fix this so telemtry goes out regardless
  cleanOptionsState.hotWaterDispenseHeaterOn = true; 

  naState.state = NA;

  currentGaggiaState = &joiningNetwork;
  manualNextGaggiaState = &naState;
}
