#include "State.h"

#include <Qwiic_Scale_NAU7802_Arduino_Library.h>

GaggiaState helloState,
            featuresState,
            wandFeaturesState,
            tareCup1State,
            measureBeansState,
            tareCup2State,
            heatingToBrewState,
            preinfusionState,
            brewingState,
            heatingToDispenseState,
            dispenseHotWaterState,
            doneBrewingState,
            heatingToSteamState, 
            steamingState,
            coolStartState,
            coolingState,
            coolDoneState,
            cleanInstructions1State,
            cleanInstructions2State,
            cleanCycle1State,
            cleanInstructions3State,
            cleanCycle2State,
            cleanDoneState,
            joiningNetwork,
            ignoringNetwork,
            naState,

            currentGaggiaState;

// After brewing, we want to message to the user and indicate that 
// they can remove their cup, but we don't want for user input
// before proceeding to heat up for steam... So we just delay a bit in
// case the uesr is watching.. 
int DONE_BREWING_LINGER_TIME_SECONDS = 5;
  

int NUMBER_OF_CLEAN_CYCLES = 20; // (10 on and off based on https://youtu.be/lfJgabTJ-bM?t=38)
int SECONDS_PER_CLEAN_CYCLE = 4; 

// It is possible to manually direct the next station transition
GaggiaState manualNextGaggiaState;

// Using all current state, we derive the next state of the system
// State transitions are documented here:
// https://docs.google.com/drawings/d/1EcaUzklpJn34cYeWsTnApoJhwBhA1Q4EVMr53Kz9T7I/edit?usp=sharing
GaggiaState getNextGaggiaState() {

  if (manualNextGaggiaState.state != NA) {
    GaggiaState nextGaggiaState = manualNextGaggiaState;
    manualNextGaggiaState = naState;

    return nextGaggiaState;
  }

  switch (currentGaggiaState.state) {

    case IGNORING_NETWORK :

      if (WiFi.isOff()) {
        return helloState;
      }

      break;

    case JOINING_NETWORK :

      if (networkState.connected) {
        return helloState;
      }
      
      if (userInputState.state == SHORT_PRESS || userInputState.state == LONG_PRESS) {
        return ignoringNetwork;
      }

      break;

    case HELLO :

      // We have to give the scale long enough to tare proper weight
      if (userInputState.state == SHORT_PRESS) {
        if (heaterState.measuredTemp >= TARGET_BREW_TEMP * 1.20) {
          return coolStartState;
        } else {
          return tareCup1State;
        }
      }

      if (userInputState.state == LONG_PRESS) {
        return featuresState;
      }
      break;

    case FEATURES :

      if (userInputState.state == SHORT_PRESS) {
        return wandFeaturesState;
      }
     
      if (userInputState.state == LONG_PRESS) {
        return cleanInstructions1State;
      }
      break;      

    case WAND_FEATURES :

      if (userInputState.state == SHORT_PRESS) {
        return heatingToDispenseState;
      }
     
      if (userInputState.state == LONG_PRESS) {
        return heatingToSteamState;
      }
      break;    

    case TARE_CUP_BEFORE_MEASURE :

      if (userInputState.state == SHORT_PRESS) {
        return measureBeansState;
      }
     
      if (userInputState.state == LONG_PRESS) {
        return helloState;
      }
      break;

    case MEASURE_BEANS :

      if (userInputState.state == SHORT_PRESS) {
        return tareCup2State;
      }
      if (userInputState.state == LONG_PRESS) {
        return helloState;
      }
      break;  

    case TARE_CUP_AFTER_MEASURE :

      if (userInputState.state == SHORT_PRESS) {
        return heatingToBrewState;
      }
      if (userInputState.state == LONG_PRESS) {
        return helloState;
      }
      break;

    case HEATING_TO_BREW :

      if (heaterState.measuredTemp >= TARGET_BREW_TEMP) {
        return preinfusionState;
      }
      if (userInputState.state == LONG_PRESS) {
        return helloState;
      }
      break;

    case PREINFUSION :

      if(scaleState.measuredWeight - scaleState.tareWeight > PREINFUSION_WEIGHT_THRESHOLD_GRAMS) {
        return brewingState;
      }
      if (userInputState.state == LONG_PRESS) {
        return helloState;
      }
      break;

    case BREWING :

      if ((scaleState.measuredWeight - scaleState.tareWeight) >= 
            scaleState.targetWeight) {
        return doneBrewingState;
      }
      if (userInputState.state == LONG_PRESS) {
        return helloState;
      }
      break;

    case DONE_BREWING :

      if ((millis() - currentGaggiaState.stateEnterTimeMillis) > 
             DONE_BREWING_LINGER_TIME_SECONDS * 1000) {        
        return heatingToSteamState;
      }

      if (userInputState.state == LONG_PRESS) {
        return helloState;
      }
      break;

    case HEATING_TO_STEAM :

      if (heaterState.measuredTemp >= TARGET_STEAM_TEMP) {
        return steamingState;
      }
      if (userInputState.state == LONG_PRESS) {
        return helloState;
      }
      break;

    case STEAMING :

      if (userInputState.state == SHORT_PRESS) {
        return helloState;
      }
      if (userInputState.state == LONG_PRESS) {
        return helloState;
      }
      break;
      
    case HEATING_TO_DISPENSE :

      if (heaterState.measuredTemp >= TARGET_HOT_WATER_DISPENSE_TEMP) {
        return dispenseHotWaterState;
      }
     
      if (userInputState.state == LONG_PRESS) {
        return helloState;
      }
      break; 

    case DISPENSE_HOT_WATER :

      if (userInputState.state == SHORT_PRESS) {
        return helloState;
      }
      if (userInputState.state == LONG_PRESS) {
        return helloState;
      }
      break;
     
    case COOL_START :

      if (userInputState.state == SHORT_PRESS) {
        return coolingState;
      }
      if (userInputState.state == LONG_PRESS) {
        return helloState;
      }
      break;

    case COOLING :

      if (userInputState.state == SHORT_PRESS) {
        return coolDoneState;
      }

      if (heaterState.measuredTemp < TARGET_BREW_TEMP) {
        return coolDoneState;
      }
      if (userInputState.state == LONG_PRESS) {
        return helloState;
      }
      break;

    case COOL_DONE :

      if (userInputState.state == SHORT_PRESS) {
        if (heaterState.measuredTemp >= TARGET_BREW_TEMP) {
          return coolStartState;
        } else {
          return helloState;
        }
      }
      if (userInputState.state == LONG_PRESS) {
        return helloState;
      }
      break;  
  
    case CLEAN_INSTRUCTION_1 :

      if (userInputState.state == SHORT_PRESS) {
        return cleanInstructions2State;
      }
     
      if (userInputState.state == LONG_PRESS) {
        return helloState;
      }
      break;

    case CLEAN_INSTRUCTION_2 :

      if (userInputState.state == SHORT_PRESS) {
        return cleanCycle1State;
      }
     
      if (userInputState.state == LONG_PRESS) {
        return helloState;
      }
      break;

    case CLEAN_SOAP :

      if (currentGaggiaState.counter == currentGaggiaState.targetCounter-1) {
        return cleanInstructions3State;
      }
     
      if (userInputState.state == LONG_PRESS) {
        return helloState;
      }
      break;

    case CLEAN_INSTRUCTION_3 :

      if (userInputState.state == SHORT_PRESS) {
        return cleanCycle2State;
      }
     
      if (userInputState.state == LONG_PRESS) {
        return helloState;
      }
      break;

    case CLEAN_RINSE :

      if (currentGaggiaState.counter == currentGaggiaState.targetCounter-1) {
        return cleanDoneState;
      }
     
      if (userInputState.state == LONG_PRESS) {
        return helloState;
      }
      break;
  
      case CLEAN_DONE :

      if (userInputState.state == SHORT_PRESS) {
        return helloState;
      }
     
      if (userInputState.state == LONG_PRESS) {
        return helloState;
      }
      break;
  
  } 

  if ((millis() - userInputState.lastUserInteractionTimeMillis) > 
    RETURN_TO_HOME_INACTIVITY_MINUTES * 60 * 1000) {
        return helloState;
  }

  return currentGaggiaState;
}  


long filterLowWeight(long weight) {
  if (abs(weight) < LOW_WEIGHT_THRESHOLD) {
    return 0;
  } else {
    return weight;
  }
}

// line starts at 1
String updateDisplayLine(char *message, 
                         int line,
                         String previousLineDisplayed) {

  display.setCursor(0,line-1);
  
  String lineToDisplay; 
  
  lineToDisplay = decodeMessageIfNecessary(message,
                                           "{adjustedWeight}/{targetBeanWeight}",
                                           filterLowWeight(scaleState.measuredWeight - scaleState.tareWeight),
                                           TARGET_BEAN_WEIGHT,
                                           " g");
  if (lineToDisplay == "") {
    lineToDisplay = decodeMessageIfNecessary(message,
                                            "{measuredBrewTemp}/{targetBrewTemp}",
                                            heaterState.measuredTemp,
                                            TARGET_BREW_TEMP,
                                            " degrees C");
    if (lineToDisplay == "") {
      lineToDisplay = decodeMessageIfNecessary(message,
                                              "{adjustedWeight}/{targetBrewWeight}",
                                              filterLowWeight(scaleState.measuredWeight - scaleState.tareWeight),
                                              scaleState.targetWeight,
                                              " g");
      if (lineToDisplay == "") {
        lineToDisplay = decodeMessageIfNecessary(message,
                                              "{measuredSteamTemp}/{targetSteamTemp}",
                                              heaterState.measuredTemp,
                                              TARGET_STEAM_TEMP,
                                              " degrees C");
        if (lineToDisplay == "") {
          lineToDisplay = decodeMessageIfNecessary(message,
                                                "{adjustedWeight}",
                                                filterLowWeight(scaleState.measuredWeight - scaleState.tareWeight),
                                                0,
                                                " g");
          if (lineToDisplay == "") {
            lineToDisplay = decodeMessageIfNecessary(message,
                                                     "{flowRate}",
                                                     waterPumpState.flowRateGPS,
                                                     0,
                                                     " g/30sec");
            if (lineToDisplay == "") {
              lineToDisplay = decodeMessageIfNecessary(message,
                                                       "{measuredBars}/{targetBars}",
                                                       waterPumpState.measuredPressureInBars,
                                                       waterPumpState.targetPressureInBars,
                                                       " bars");
              if (lineToDisplay == "") {
                // One count consists of both the clean and rest... so we only have
                // targetCounter/2 total clean passes.
                lineToDisplay = decodeMessageIfNecessary(message,
                                                       "{currentPass}/{targetPass}",
                                                       (currentGaggiaState.counter/2)+1,
                                                       (currentGaggiaState.targetCounter)/2,
                                                       " pass");
                if (lineToDisplay == "") {
                  // One count consists of both the clean and rest... so we only have
                  // targetCounter/2 total clean passes.
                  lineToDisplay = decodeMessageIfNecessary(message,
                                                       "{pumpDutyCycle}/{maxDutyCycle}",
                                                       waterPumpState.pumpDutyCycle,
                                                       MAX_PUMP_DUTY_CYCLE,
                                                       " %");
                  if (lineToDisplay == "") {
                    lineToDisplay = decodeMessageIfNecessary(message,
                                              "{measuredSteamTemp}/{targetHotWaterDispenseTemp}",
                                              heaterState.measuredTemp,
                                              TARGET_HOT_WATER_DISPENSE_TEMP,
                                              " degrees C");
                  }
                }
              }   
            }          
          }
        }      
      }
    }
  }

  if (lineToDisplay == "") {
    lineToDisplay = String(message);
  }

  if (line == 1) {
    // inject 'heating' indicator
    if (isHeaterOn()) {
      lineToDisplay.setCharAt(19, '*');
    }
  }

  // This prevents flashing
  if (lineToDisplay != previousLineDisplayed) {
    display.print(lineToDisplay);
  }

  return lineToDisplay;
}


char* getStateName(int stateEnum) {
   switch (stateEnum) {
    case HELLO: return "hello";
    case FEATURES: return "features";
    case WAND_FEATURES: return "wandFeatures";
    case TARE_CUP_BEFORE_MEASURE: return "tareCupBeforeMeasure";
    case MEASURE_BEANS: return "measureBeans";
    case TARE_CUP_AFTER_MEASURE: return "tareCupAfterMeasure";
    case HEATING_TO_BREW: return "heating";
    case PREINFUSION: return "preInfusion";
    case BREWING: return "brewing";
    case DONE_BREWING: return "doneBrewing";
    case HEATING_TO_STEAM: return "heatingToSteam";
    case STEAMING: return "steaming";
    case COOL_START: return "coolStart";
    case COOLING: return "cooling";
    case COOL_DONE: return "coolDone";
    case CLEAN_INSTRUCTION_1: return "cleanInst1";
    case CLEAN_INSTRUCTION_2: return "cleanInst2";
    case CLEAN_SOAP: return "cleanSoap";
    case CLEAN_INSTRUCTION_3: return "cleanInst3";
    case CLEAN_RINSE: return "cleanRinse";
    case CLEAN_DONE: return "cleanDone";
    case HEATING_TO_DISPENSE: return "heatingToDispense";
    case DISPENSE_HOT_WATER: return "dispenseHotWater";
    case IGNORING_NETWORK: return "ignoringNetwork";
    case JOINING_NETWORK: return "joingingNetwork";
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
}

void processCurrentGaggiaState() { 

  // 
  // Process Display
  //
  displayState.display1 = updateDisplayLine(currentGaggiaState.display1, 
                                             1, 
                                             displayState.display1);

  displayState.display2 = updateDisplayLine(currentGaggiaState.display2, 
                                             2, 
                                             displayState.display2);

  displayState.display3 = updateDisplayLine(currentGaggiaState.display3, 
                                             3, 
                                             displayState.display3);

  displayState.display4 = updateDisplayLine(currentGaggiaState.display4, 
                                             4, 
                                             displayState.display4);

  long nowTimeMillis = millis();

  // Process Heaters
  //
  // Even though a heater may be 'on' during this state, the control system
  // for the heater turns it off and on intermittently in an attempt to regulate
  // the temperature around the target temp.
  if (currentGaggiaState.brewHeaterOn) {
    readBrewHeaterState();  

    if (shouldTurnOnHeater()) {
      turnHeaterOn();
    } else {
      turnHeaterOff();
    } 
  }
  if (currentGaggiaState.steamHeaterOn) {
    readSteamHeaterState(); 

    if (shouldTurnOnHeater()) {
      turnHeaterOn();
    } else {
      turnHeaterOff();
    } 
  }
  if (currentGaggiaState.hotWaterDispenseHeaterOn) {
    readSteamHeaterState(); 

    if (shouldTurnOnHeater()) {
      turnHeaterOn();
    } else {
      turnHeaterOff();
    } 
  }
  if (!currentGaggiaState.brewHeaterOn && !currentGaggiaState.steamHeaterOn && !currentGaggiaState.hotWaterDispenseHeaterOn) {
      turnHeaterOff();
  }

  if (currentGaggiaState.fillingReservoir) {

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

  if (currentGaggiaState.measureTemp) {
    readBrewHeaterState();
  }

  if (currentGaggiaState.waterThroughGroupHead || currentGaggiaState.waterThroughWand) {
    if (currentGaggiaState.state == CLEAN_SOAP || currentGaggiaState.state == CLEAN_RINSE) {
      // whether or not we dispense water depends on where we are in the clean cycle...      

      currentGaggiaState.targetCounter = NUMBER_OF_CLEAN_CYCLES;

      if (currentGaggiaState.counter % 2 == 0) { // 0, and even counts 0, 2, 4...
        // we dispense water.. this fills portafilter with pressured hot water...
        startDispensingWater(true); // going through grouphead
      } else { // odd counts 1,3,5...
        // we stop dispensing water .. this backfushes hot water through group head...
        stopDispensingWater();
      }
  
      // when we first enter state, stopTimeMillis is -1, so this condition is true
      if (nowTimeMillis > currentGaggiaState.stopTimeMillis) {
                
        // when we first enter this state, counter is -1, so it gets bumped to 0.
        currentGaggiaState.counter = currentGaggiaState.counter + 1; 
        
        // start new cleaning cycle
        double timeMultiplier = 1;
        if (currentGaggiaState.counter % 2 != 0) {
          // odd, or off, cycles should be shorter
          timeMultiplier = .2;
        }
        currentGaggiaState.stopTimeMillis = nowTimeMillis + SECONDS_PER_CLEAN_CYCLE*timeMultiplier*1000;
      }
    } else {
      // normal dispense state
      startDispensingWater(currentGaggiaState.waterThroughGroupHead);
    }
  } else {
    stopDispensingWater();
  }

  if (currentGaggiaState.state == IGNORING_NETWORK) {

    // we do this just incase we spipped network AFTER
    // we achieved a connection.
    networkState.connected = false;
    Particle.disconnect();
    WiFi.off();
  }

  if (currentGaggiaState.state == JOINING_NETWORK) {

    if (WiFi.connecting()) {
      return;
    }

    if (!Particle.connected()) {
      Particle.connect(); 
    } else {
      networkState.connected = true;
    }
  }

  if (currentGaggiaState.state == BREWING || currentGaggiaState.state == PREINFUSION) {
    if (updateFlowRateMetricWhenItsTime()) {
      sendTelemetry();
    }
  }

  #ifdef AIO_USERNAME
    if (currentGaggiaState.state == HELLO) {
      // when we enter hello, we disconnect from MQTT broker for telemetry...
      if (networkState.connected) {
        // recall the system returns to hello after 15 minutes of inactivity.
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
  if (currentGaggiaState.tareScale) {
    scaleState.tareWeight = scaleState.measuredWeight;
  }

  // Process Record Weight 
  if (currentGaggiaState.recordWeight) {
    scaleState.targetWeight = 
      (scaleState.measuredWeight - scaleState.tareWeight)*BREW_WEIGHT_TO_BEAN_RATIO; 
  }

  // This gives our current state one last chance to log any telemetry.
  if (currentGaggiaState.state == BREWING) {
    sendTelemetry();
  }

  // Things we always reset when leaving a state...
  currentGaggiaState.stopTimeMillis = -1;
  currentGaggiaState.counter = -1;
  nextFlowRateSampleMillis = -1;
}

String readCurrentState() {
  return String(currentGaggiaState.state);
}

int setHeatingState(String _) {
  
  // by jumping to this state, we are bypassing earlier states.. so we need to
  // fake any data that should have been collected.
  scaleState.tareWeight = 320;
  scaleState.targetWeight = 100;

  manualNextGaggiaState = heatingToBrewState;
  return 1;
}

int setCoolingState(String _) {
  manualNextGaggiaState = coolingState;
  return 1;
}

int setSteamingState(String _) {
  manualNextGaggiaState = steamingState;
  return 1;
}

int setHelloState(String _) {
  manualNextGaggiaState = helloState;
  return 1;
}

int setBrewingState(String _) {
  
  // by jumping to this state, we are bypassing earlier states.. so we need to
  // fake any data that should have been collected.
  scaleState.tareWeight = 320;
  scaleState.targetWeight = 100;

  manualNextGaggiaState = brewingState;
  return 1;
}

int setPreInfusionState(String _) {
  
  // by jumping to this state, we are bypassing earlier states.. so we need to
  // fake any data that should have been collected.
  scaleState.tareWeight = 320;
  scaleState.targetWeight = 100;

  manualNextGaggiaState = preinfusionState;
  return 1;
}

int setDispenseHotWater(String _) {
  manualNextGaggiaState = dispenseHotWaterState;
  return 1;
}


void stateInit() {

  Particle.variable("currentState", readCurrentState);

  Particle.function("setDispenseHotWater", setDispenseHotWater);
  Particle.function("setPreinfusionState", setPreInfusionState);
  Particle.function("setHeatingState", setHeatingState);
  Particle.function("setBrewingState", setBrewingState);
  Particle.function("setSteamingState", setSteamingState);
  //Particle.function("setCoolingState", setCoolingState);
  Particle.function("setHelloState", setHelloState);

 // Define all possible states of RoboGaggia
  helloState.state = HELLO;
  //helloState.state = HELLO; 
  helloState.display1 =            "Hi.                 ";
  helloState.display2 =            "Clear scale surface.";
  helloState.display3 =            "Click to Brew,      ";
  helloState.display4 =            "Hold for Features   ";
  
  // We need to measure temp when leaving to know if we need to
  // do cooling phase first...
  helloState.measureTemp = true;

  // we don't want to heat here in case the unit was turned on and
  // person walked away for a few days ...
  helloState.brewHeaterOn = false; 

  // we tare here so the weight of the scale itself isn't shown
  // when we are measuring things...
  helloState.tareScale = true; 

  featuresState.state = FEATURES; 
  featuresState.display1 =            "Select Feature      ";
  featuresState.display2 =            "                    ";
  featuresState.display3 =            "Click for wand,     ";
  featuresState.display4 =            "Hold for Clean      ";
  featuresState.fillingReservoir = true;

  wandFeaturesState.state = WAND_FEATURES; 
  wandFeaturesState.display1 =        "Select Wand Feature ";
  wandFeaturesState.display2 =        "Click for           ";
  wandFeaturesState.display3 =        "  hot water,        ";
  wandFeaturesState.display4 =        "Hold for Steam      ";
  wandFeaturesState.fillingReservoir = true;

  tareCup1State.state = TARE_CUP_BEFORE_MEASURE; 
  tareCup1State.display1 =         "Place empty cup     ";
  tareCup1State.display2 =         "on tray.            ";
  tareCup1State.display3 =         "{adjustedWeight}";
  tareCup1State.display4 =         "Click when Ready    ";
  tareCup1State.tareScale = true; 
  tareCup1State.brewHeaterOn = true; 
  tareCup1State.fillingReservoir = true;

  measureBeansState.state = MEASURE_BEANS; 
  measureBeansState.display1 =     "Add beans to cup.   ";
  measureBeansState.display2 =     "{adjustedWeight}/{targetBeanWeight}";
  measureBeansState.display3 =     "{measuredBrewTemp}/{targetBrewTemp}";
  measureBeansState.display4 =     "Click when Ready    ";
  measureBeansState.recordWeight = true; 
  measureBeansState.brewHeaterOn = true; 

  tareCup2State.state = TARE_CUP_AFTER_MEASURE; 
  tareCup2State.display1 =         "Grind & load beans, ";
  tareCup2State.display2 =         "Place empty cup     ";
  tareCup2State.display3 =         "back on tray.       ";
  tareCup2State.display4 =         "Click when Ready    ";
  tareCup2State.tareScale = true; 
  tareCup2State.brewHeaterOn = true; 

  heatingToBrewState.state = HEATING_TO_BREW; 
  heatingToBrewState.display1 =    "Heating to brew.    ";
  heatingToBrewState.display2 =    "Leave cup on tray.  ";
  heatingToBrewState.display3 =    "{measuredBrewTemp}/{targetBrewTemp}";
  heatingToBrewState.display4 =    "Please wait ...     ";
  heatingToBrewState.brewHeaterOn = true; 

  preinfusionState.state = PREINFUSION; 
  preinfusionState.display1 =          "Infusing coffee.    ";
  preinfusionState.display2 =          "{measuredBars}/{targetBars}";
  preinfusionState.display3 =          "{measuredBrewTemp}/{targetBrewTemp}";
  preinfusionState.display4 =          "Please wait ...     ";
  preinfusionState.waterThroughGroupHead = true; 
  preinfusionState.brewHeaterOn = true; 

  brewingState.state = BREWING; 
  brewingState.display1 =          "Brewing.            ";
  brewingState.display2 =          "{adjustedWeight}/{targetBrewWeight}";
  brewingState.display3 =          "{measuredBars}/{targetBars}";
  brewingState.display4 =          "{pumpDutyCycle}/{maxDutyCycle}";
  brewingState.brewHeaterOn = true; 
  brewingState.waterThroughGroupHead = true; 

  heatingToDispenseState.state = HEATING_TO_DISPENSE; 
  heatingToDispenseState.display1 =    "Heating to dispense ";
  heatingToDispenseState.display2 =    "          hot water.";
  heatingToDispenseState.display3 =   "{measuredSteamTemp}/{targetHotWaterDispenseTemp}";
  heatingToDispenseState.display4 =    "Please wait ...     ";
  heatingToDispenseState.hotWaterDispenseHeaterOn = true; 

  dispenseHotWaterState.state = DISPENSE_HOT_WATER; 
  dispenseHotWaterState.display1 =          "Use steam wand to  ";
  dispenseHotWaterState.display2 =          "dispense hot water. ";
  dispenseHotWaterState.display3 =          "                    ";
  dispenseHotWaterState.display4 =          "Click when Done     ";
  dispenseHotWaterState.hotWaterDispenseHeaterOn = true; 
  dispenseHotWaterState.waterThroughWand = true; 

  doneBrewingState.state = DONE_BREWING; 
  doneBrewingState.display1 =      "Done brewing.       ";
  doneBrewingState.display2 =      "                    ";
  doneBrewingState.display3 =      "Remove cup.         ";
  doneBrewingState.display4 =      "Please wait ...     ";
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

  cleanInstructions1State.state = CLEAN_INSTRUCTION_1;
  cleanInstructions1State.display1 =            "Load backflush      ";
  cleanInstructions1State.display2 =            "portafilter with    ";
  cleanInstructions1State.display3 =            "3g of Cafiza.       ";
  cleanInstructions1State.display4 =            "Click when Ready    ";

  cleanInstructions2State.state = CLEAN_INSTRUCTION_2;
  cleanInstructions2State.display1 =            "Remove scale        ";
  cleanInstructions2State.display2 =            "from the drain pan. ";
  cleanInstructions2State.display3 =            "                    ";
  cleanInstructions2State.display4 =            "Click when Ready    ";

  cleanCycle1State.state = CLEAN_SOAP;
  cleanCycle1State.display1 =            "Backflushing        ";
  cleanCycle1State.display2 =            "with cleaner.       ";
  cleanCycle1State.display3 =            "{measuredBars}/{targetBars}";
  cleanCycle1State.display4 =            "{currentPass}/{targetPass}";
  cleanCycle1State.brewHeaterOn = true; 
  cleanCycle1State.waterThroughGroupHead = true; 
  // trying to fill reservoir during clean seemed to cause problems..
  // not sure why

  cleanInstructions3State.state = CLEAN_INSTRUCTION_3;
  cleanInstructions3State.display1 =            "Clean out backflush ";
  cleanInstructions3State.display2 =            "portafilter.        ";
  cleanInstructions3State.display3 =            "                    ";
  cleanInstructions3State.display4 =            "Click when Ready    ";

  cleanCycle2State.state = CLEAN_RINSE;
  cleanCycle2State.display1 =            "Backflushing        ";
  cleanCycle2State.display2 =            "with water.         ";
  cleanCycle2State.display3 =            "{measuredBars}/{targetBars}";
  cleanCycle2State.display4 =            "{currentPass}/{targetPass}";
  cleanCycle2State.brewHeaterOn = true; 
  cleanCycle2State.waterThroughGroupHead = true; 
  // trying to fill reservoir during clean seemed to cause problems..
  // not sure why

  cleanDoneState.state = CLEAN_DONE;
  cleanDoneState.display1 =            "Replace normal      ";
  cleanDoneState.display2 =            "portafilter.        ";
  cleanDoneState.display3 =            "Return scale.       ";
  cleanDoneState.display4 =            "Click when Done     ";

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


  naState.state = NA;

  currentGaggiaState = joiningNetwork;
  manualNextGaggiaState = naState;
}
