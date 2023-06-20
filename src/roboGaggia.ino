/**
Copyright [2022] [Nicholas J. DiPatri]

Please give me attribution (keep my name somewhere) if you use this code in part or whole. Thanks!

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    https://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

Also, some parts of the PID controller software was derived from SparkFun.com:

Copyright (c) 2016 SparkFun Electronics
**/

#include "components/Scale.h"
#include "components/WaterPump.h"
#include "components/UserInput.h"
#include "components/Heater.h"
#include "components/WaterReservoir.h"
#include "components/Display.h"
#include "components/State.h"


#define LOOP_INTERVAL_MILLIS 75


SerialLogHandler logHandler;
SYSTEM_THREAD(ENABLED);

// This instructs the core to not connect to the
// Particle cloud until explicitly instructed to
// do so from within our service loop ... This is so the device
// has the ability to operate offline.
SYSTEM_MODE(MANUAL);

// setup() runs once, when the device is first turned on.
void setup() {
  
  // I2C Setup
  Wire.begin();

  // Manages system state, when to change state, and what to do when
  // entering or leaving state.  
  stateInit();

  // Manages the vibration pump.  Maintains PID controllers for
  // both flow-based control (brewing) and 
  // pressure-based control (preinfusion, hot water dispense, cleaning)
  waterPumpInit();

  // Manages the water reservoir to ensure water always available.  Controls the
  // solenoid valve attached to external water feed. 
  waterReservoirInit();

  // Manages the custom-built scale embedded in the drain pan.  This scale has a 
  // single 500g capacity load cell.
  scaleInit();

  // Manages user input from the single arcade-style button.  This button is
  // capable of 'short' and 'long' press operations.
  userInputInit();

  heaterInit();

  // Manages 4x24 LCD Display and maintains existing state so we only write to
  // LCD when text has changed.
  displayInit();

  // 
  networkInit();

  // Provides core data types and logging
  commonInit();

  // Wait for a USB serial connection for up to 3 seconds
  waitFor(Serial.isConnected, 3000);
}

boolean first = true;
void loop() {
  
  readScaleState();
  
  readUserInputState();

  // Determine next Gaggia state based on inputs and current state ...
  // (e.g. move to 'Done Brewing' state once target weight is achieved, etc.)
  GaggiaState* nextGaggiaState = getNextGaggiaState();

  if (first || nextGaggiaState->state != currentGaggiaState->state) {

    // Perform actions given current Gaggia state and input ...
    // This step does also mutate current state
    // (e.g. record weight of beans, tare measuring cup)

    // Things we do when we leave a state
    processOutgoingGaggiaState();
  
      // Things we do when we enter a state
    processIncomingGaggiaState(nextGaggiaState);
  
    nextGaggiaState->stateEnterTimeMillis = millis();
    currentGaggiaState->stateExitTimeMillis = millis();
  }

  currentGaggiaState = nextGaggiaState;

  // Things we do when we are within a state 
  processCurrentGaggiaState();

  // resume service loop
  if (networkState.connected) {
    Particle.process();
  }

  if (isInTestMode) {
    delay(2000);
  } else {
    delay(LOOP_INTERVAL_MILLIS);
  }

  pingMQTTIfNecessary();

  first = false;
}