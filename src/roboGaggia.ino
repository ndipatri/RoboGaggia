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

#include <Wire.h> 
#include <string.h> 
#include <SPI.h>
#include <SerLCD.h>
#include <Qwiic_Scale_NAU7802_Arduino_Library.h>
#include <SparkFun_Qwiic_Button.h>
#include <Adafruit_ADS1X15.h>
#include <pid.h>

SerialLogHandler logHandler;
SYSTEM_THREAD(ENABLED);

//
// PIN Definitions
//

// Chip Select! - tie low to turn on MAX6675 and 
// get a temperature reading
#define MAX6675_CS_brew   D2 
#define MAX6675_CS_steam  D5

// Serial Clock - when brought high, shifts
// another byte from the MAX6675.
// shared between both brew and steam MAX6675 chips
// as we don't meaure simultaneously
#define MAX6675_SCK D3

// Serial Data Out
#define MAX6675_SO_brew   D4 
#define MAX6675_SO_steam  D6

// CS, SCK, and SO together are used to read serial data
// from the MAX6675 


// The TRIAC on/off signal for the AC Potentiometer
// https://rocketcontroller.com/product/1-channel-high-load-ac-dimmer-for-use-witch-micro-controller-3-3v-5v-logic-ac-50-60hz/
// NOTE: Sendin this high triggers the TRIAC (turns it on and allows current flow), but at each zero crossing, the TRIAC
// resets itself and stop current flow.
#define DISPENSE_POT D7 

// This provides a signal to help us time how to 'shape' the AC wave powering the water pump.
// The zero crossings tell us when we can disable part of the AC wave to modulate the power.
// 'Pulse Shape Modulation' (PSM)
#define ZERO_CROSS_DISPENSE_POT A2  

#define HEATER D8

// Output - For dispensing water
// IT IS EXPECTED FOR THIS TO ALSO BE TIED TO THE WATER_SENSOR
// POWER PIN... AS WE EXPECT TO TAKE WATER LEVEL MEASUREMENTS WHILE
// WE ARE DISPENSING WATER
//
// Also, since we use the AC_DIMMER (via DISPENSE_POT), merely pulling
// this DISPENSE_WATER pin high does NOT start dispensing water... the
// POT also has to be pulled HIGH for that to happen.
#define DISPENSE_WATER  TX

// 0 - low water mark, ~500 high water mark
#define WATER_RESERVOIR_SENSOR  A1

// send high to turn on solenoid
#define WATER_RESERVOIR_SOLENOID  RX 


//
// Constants
//

double TARGET_BREW_TEMP = 103; // should be 65
double TOO_HOT_TO_BREW_TEMP = 110; // should be 80
double TARGET_STEAM_TEMP = 140; // should be 80
double TARGET_BEAN_WEIGHT = 22; // grams

// How long the button has to be pressed before it is considered
// a long press
float BUTTON_LONG_PRESS_DURATION_MILLIS = 1000;

// This is the ration of final espresso weight compared to
// that of the ground beans.  Typically this is 2-to-1
float BREW_WEIGHT_TO_BEAN_RATIO = 2.0;

// LOAD_BLOCK_READING = (SCALE_FACTOR)(ACTUAL_WEIGHT_IN_GRAMS) + SCALE_OFFSET 
//
// ACTUAL_WEIGHT_IN_GRAMS = LOAD_BLOCK_READING/SCALE_FACTOR - SCALE_OFFSET
double SCALE_FACTOR = 3137;  
double SCALE_OFFSET = 47;

// Below which we consider weight to be 0
int LOW_WEIGHT_THRESHOLD = 4;

// This is for implementing a schmitt trigger control system for
// water reservoir sensor
// These values were imperically derived by measuring these levels
// while immersing the water sensor.
// These values need to be recalibrated everytime you replace sensor
int LOW_WATER_RESEVOIR_LIMIT = 100;
int HIGH_WATER_RESEVOIR_LIMIT = 300;

int RETURN_TO_HOME_INACTIVITY_MINUTES = 10;

// How long we will the portafilter with semi-hot water
int PREINFUSION_DURATION_SECONDS = 6;
  
int DONE_BREWING_LINGER_TIME_SECONDS = 5;
  
// How many 60Hz cycles we consider in our flow rate
// So 10 means we count 10 cycles before we reset our count.  We could
// then desire a 50% duty cycle, which would mean 5 cycles on and 
// 5 cycles off
int DISPENSE_FLOW_RATE_TOTAL_CYCES = 20;

// NJD TODO PRESSURE - Until I get the actual sensor hooked up, i dont know what these absolute
// values really are...
double DISPENSING_PSI = 9.0;
double PRE_INFUSION_PSI = 2.0;
// see https://docs.google.com/spreadsheets/d/1_15rEy-WI82vABUwQZRAxucncsh84hbYKb2WIA9cnOU/edit?usp=sharing
// as shown in shart above, the following values were derived by hooking up a bicycle pump w/ guage to the
// pressure sensor and measuring a series of values vs bar pressure. 
double PRESSURE_SENSOR_SCALE_FACTOR = 1658.0;
int PRESSURE_SENSOR_OFFSET = 2470; 

//
// State
//

struct WaterPumpState {

double targetPressureInBars = PRE_INFUSION_PSI; 

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

} waterPumpState;
// Measures the current pressure in the water pump system
void readPumpState(WaterPumpState *waterPumpState);
Adafruit_ADS1115 ads1115;  // the adc for the pressure sensor


struct WaterReservoirState {

  int measuredWaterLevel = 0;

  boolean isSolenoidOn = false;

} waterReservoirState;
boolean doesWaterReservoirNeedFilling(int CHIP_ENABLE_PIN, int ANALOG_INPUT_PIN, WaterReservoirState *waterReservoirState);


struct ScaleState {

  long measuredWeight = 0;

  // this will be the measuredWeight - tareWeight * BREW_WEIGHT_TO_BEAN_RATIO
  // at the moment this value is recorded...
  long targetWeight = 0; 

  // recorded weight of cup meant be used when
  // measuring the weight of beans or brew  
  long tareWeight = 0;

  // Units are grams/30-seconds
  float flowRate = 0;

} scaleState;
void readScaleState(NAU7802 myScale, ScaleState *scaleState);
NAU7802 myScale; //Create instance of the NAU7802 class


struct HeaterState {
  boolean thermocoupleError = false;  

  // current temp
  // this is  updated as we read thermocouple sensor
  double measuredTemp;
  
  // This is calculated and updated by the PID
  double heaterDurationMillis;

  // Used to track ongoing heat cycles...
  float heaterStarTime = -1;

  // The control system for determining when to turn on
  // the heater in order to achieve target temp
  PID *heaterPID;

} heaterState;
void readHeaterState(int CHIP_SELECT_PIN, int SERIAL_OUT_PIN, int SERIAL_CLOCK_PIN, HeaterState *heaterState);
boolean shouldTurnOnHeater(float nowTimeMillis, HeaterState *heaterState);


// Based on the physical button, we derive one of three
// input states
enum UserInputStateEnum {
  IDLE = 0,
  SHORT_PRESS = 1,
  LONG_PRESS = 2 
};
struct UserInputState {
  enum UserInputStateEnum state;
  // We need to evaluate the state of a button over time to determine
  // if it's a short press or a long press
  float buttonPressStartTimeMillis = -1;

  float lastUserInteractionTimeMillis = -1;
} userInputState;
void readUserInputState(boolean isButtonPressedRightNow, float nowTimeMillis, UserInputState *userInputState);
QwiicButton userButton;


struct DisplayState {
  String display1;
  String display2;
  String display3;
  String display4;

} displayState;
String updateDisplayLine(char *message, 
                        int line,
                        HeaterState *heaterState,
                        ScaleState *scaleState,
                        WaterPumpState *waterPumpState,
                        String previousLineDisplayed);
SerLCD display; // Initialize the library with default I2C address 0x72


enum GaggiaStateEnum {
  HELLO = 0, 
  TARE_CUP_1 = 1, 
  MEASURE_BEANS = 2,  
  TARE_CUP_2 = 3, 
  HEATING = 4, 
  PREINFUSION = 5, 
  BREWING = 6, 
  DONE_BREWING = 7, 
  HEATING_TO_STEAM = 8, 
  STEAMING = 9, 
  COOL_START = 10, 
  COOLING = 11, 
  COOL_DONE = 12, 
  NA = 13
};
struct GaggiaState {
   enum GaggiaStateEnum state;   
   char *display1 = "";
   char *display2 = "";
   char *display3 = "";
   char *display4 = "";
   boolean  waterReservoirSolenoidOn = false;
   
   // WARNING: this cannot be done at the same time (or even in an adjacent state)
   // to dispensing water.
   boolean  fillingReservoir = false;

   boolean  brewHeaterOn = false;
   boolean  steamHeaterOn = false;
   boolean  measureTemp = false;
   boolean  tareScale = false;
   boolean  dispenseWater = false;
   boolean  recordWeight = false;
   float stateEnterTimeMillis = -1;
} 
helloState,
tareCup1State,
measureBeansState,
tareCup2State,
heatingState,
preinfusionState,
brewingState,
doneBrewingState,
heatingToSteamState,
steamingState,
coolStartState,
coolingState,
coolDoneState,
naState,

currentGaggiaState;

boolean isInTestMode = false;

// It is possible to manually direct the next station transition
GaggiaState manualNextGaggiaState;

// Using all current state, we derive the next state of the system
GaggiaState getNextGaggiaState(GaggiaState *currentGaggiaState, 
                               HeaterState *heaterState,
                               ScaleState *scaleState,
                               UserInputState *userInputState,
                               WaterReservoirState *waterReservoirState,
                               WaterPumpState *waterPumpState);

// Once we know the state, we affect appropriate change in our
// system attributes (e.g. temp, display)
// Things we do while we are in a state 
void processCurrentGaggiaState(GaggiaState *currentGaggiaState,
                               HeaterState *heaterState,
                               ScaleState *scaleState,
                               DisplayState *displayState,
                               WaterReservoirState *waterReservoirState,
                               WaterPumpState *waterPumpState,
                               float nowTimeMillis);
// Things we do when we leave a state
void processOutgoingGaggiaState(GaggiaState *currentGaggiaState,
                                GaggiaState *nextGaggiaState, 
                                HeaterState *heaterState,
                                ScaleState *scaleState,
                                DisplayState *displayState,
                                WaterReservoirState *waterReservoirState,
                                WaterPumpState *waterPumpState,
                                float nowTimeMillis);
// Things we do when we enter a state
void processIncomingGaggiaState(GaggiaState *currentGaggiaState,
                                GaggiaState *nextGaggiaState, 
                                HeaterState *heaterState,
                                ScaleState *scaleState,
                                DisplayState *displayState,
                                WaterReservoirState *waterReservoirState,
                                WaterPumpState *waterPumpState,
                                float nowTimeMillis);
GaggiaState returnToHelloState(ScaleState *scaleState);

// setup() runs once, when the device is first turned on.
void setup() {
  
  // I2C Setup
  Wire.begin();

  ads1115.begin();// Initialize ads1015 at the default address 0x48
  // use hte following if x48 is already taken
  //ads1115.begin(0x49);  // Initialize ads1115 at address 0x49

  // Scale check
  if (myScale.begin() == false)
  {
    Log.error("Scale not detected!");
  }
  
  if (userButton.begin() == false) {
    Log.error("Device did not acknowledge! Freezing.");
  }
  Log.error("Button acknowledged.");

  
  // setup MAX6675 to read the temperature from thermocouple
  pinMode(MAX6675_CS_brew, OUTPUT);
  pinMode(MAX6675_CS_steam, OUTPUT);
  pinMode(MAX6675_SO_brew, INPUT);
  pinMode(MAX6675_SO_steam, INPUT);
  pinMode(MAX6675_SCK, OUTPUT);
  
  // external heater elements
  pinMode(HEATER, OUTPUT);
  
  // Water Pump Potentiometer
  pinMode(DISPENSE_POT, OUTPUT);
  pinMode(ZERO_CROSS_DISPENSE_POT, INPUT_PULLDOWN);

  // water dispenser
  pinMode(DISPENSE_WATER, OUTPUT);

  pinMode(WATER_RESERVOIR_SOLENOID, OUTPUT);


  // // Useful state exposed to Particle web console
  Particle.variable("heaterTempC", heaterState.measuredTemp);
  Particle.variable("targetBrewTempC", TARGET_BREW_TEMP);
  Particle.variable("isDispensingWater",  currentGaggiaState.dispenseWater);
  Particle.variable("thermocoupleError",  heaterState.thermocoupleError);
  Particle.variable("heaterDurationMillis",  heaterState.heaterDurationMillis);
  Particle.variable("targetPressureInBars", _targetPressureInBars);
  Particle.variable("currentPressureInBars", _measuredPressureInBars);
  Particle.variable("isInTestMode",  isInTestMode);
  Particle.variable("waterLevel",  _readWaterLevel);
  Particle.variable("currentState", _readCurrentState);

  Particle.function("turnOnTestMode", turnOnTestMode);
  Particle.function("turnOffTestMode", turnOffTestMode);

  Particle.function("setHeatingState", _setHeatingState);
  Particle.function("setBrewingState", _setBrewingState);
  Particle.function("setCoolingState", _setCoolingState);
  Particle.function("setHelloState", _setHelloState);

  // Define all possible states of RoboGaggia
  helloState.state = HELLO; 
  helloState.display1 =            "Hi.                 ";
  helloState.display2 =            "Clear scale surface.";
  helloState.display3 =            "Click to Brew,      ";
  helloState.display4 =            "Hold for Steam      ";
  
  helloState.measureTemp = true;

  // we don't want to heat here in case the unit was turned on and
  // person walked away for 10 hours
  helloState.brewHeaterOn = false; 

  // we tare here so the weight of the scale itself isn't shown
  // when we are measuring things...
  helloState.tareScale = true; 

  tareCup1State.state = TARE_CUP_1; 
  tareCup1State.display1 =         "Place empty cup     ";
  tareCup1State.display2 =         "on tray.            ";
  tareCup1State.display3 =         "{measuredWeight}";
  tareCup1State.display4 =         "Click when Ready    ";
  tareCup1State.tareScale = true; 
  tareCup1State.brewHeaterOn = true; 

  measureBeansState.state = MEASURE_BEANS; 
  measureBeansState.display1 =     "Add beans to cup.   ";
  measureBeansState.display2 =     "{adjustedWeight}/{targetBeanWeight}";
  measureBeansState.display3 =     "{measuredBrewTemp}/{targetBrewTemp}";
  measureBeansState.display4 =     "Click when Ready    ";
  measureBeansState.recordWeight = true; 
  measureBeansState.brewHeaterOn = true; 

  tareCup2State.state = TARE_CUP_2; 
  tareCup2State.display1 =         "Grind & load beans, ";
  tareCup2State.display2 =         "Place empty cup     ";
  tareCup2State.display3 =         "back on tray.       ";
  tareCup2State.display4 =         "Click when Ready    ";
  tareCup2State.tareScale = true; 
  tareCup2State.brewHeaterOn = true; 

  heatingState.state = HEATING; 
  heatingState.display1 =    "Heating to brew.    ";
  heatingState.display2 =    "Leave cup on tray.  ";
  heatingState.display3 =    "{measuredBrewTemp}/{targetBrewTemp}";
  heatingState.display4 =    "Please wait ...     ";
  heatingState.brewHeaterOn = true; 

  preinfusionState.state = PREINFUSION; 
  preinfusionState.display1 =          "Infusing coffee.    ";
  preinfusionState.display2 =          "{measuredBars}/{targetBars}";
  preinfusionState.display3 =          "{measuredBrewTemp}/{targetBrewTemp}";
  preinfusionState.display4 =          "Please wait ...     ";
  preinfusionState.dispenseWater = true; 
  preinfusionState.brewHeaterOn = true; 

  brewingState.state = BREWING; 
  brewingState.display1 =          "Brewing.            ";
  brewingState.display2 =          "{adjustedWeight}/{targetBrewWeight}";
  brewingState.display3 =          "{measuredBars}/{targetBars}";
  brewingState.display4 =          "Please wait ...     ";
  brewingState.dispenseWater = true; 

  doneBrewingState.state = DONE_BREWING; 
  doneBrewingState.display1 =      "Done brewing.       ";
  doneBrewingState.display2 =      "{flowRate}";
  doneBrewingState.display3 =      "Remove cup.         ";
  doneBrewingState.display4 =      "Please wait ...     ";
  doneBrewingState.brewHeaterOn = true; 
  doneBrewingState.fillingReservoir = true;

  heatingToSteamState.state = HEATING_TO_STEAM; 
  heatingToSteamState.display1 =   "Heating to steam.   ";
  heatingToSteamState.display2 =   "{measuredSteamTemp}/{targetSteamTemp}";
  heatingToSteamState.display3 =   "{flowRate}";
  heatingToSteamState.display4 =   "Please wait ...     ";
  heatingToSteamState.steamHeaterOn = true; 

  steamingState.state = STEAMING; 
  steamingState.display1 =         "Operate steam wand. ";
  steamingState.display2 =         "{measuredSteamTemp}/{targetSteamTemp}";
  steamingState.display3 =         "{flowRate}";
  steamingState.display4 =         "Click when Done     ";
  steamingState.steamHeaterOn = true; 

  coolStartState.state = COOL_START; 
  coolStartState.display1 =        "Too hot to brew,    ";
  coolStartState.display2 =        "Need to run water.  ";
  coolStartState.display3 =        "Place cup on tray.  ";
  coolStartState.display4 =        "Click when Ready    ";

  coolingState.state = COOLING; 
  coolingState.display1 =          "Dispensing to cool  ";
  coolingState.display2 =          "{measuredBrewTemp}/{targetBrewTemp}";
  coolingState.display3 =          "                    ";
  coolingState.display4 =          "Click to Stop       ";
  coolingState.dispenseWater = true; 
  coolingState.measureTemp = true;

  coolDoneState.state = COOL_DONE;
  coolDoneState.display1 =         "Discard water.      ";
  coolDoneState.display2 =         "                    ";
  coolDoneState.display3 =         "                    ";
  coolDoneState.display4 =         "Click when Ready    ";

  naState.state = NA;

  currentGaggiaState = helloState;
  manualNextGaggiaState = naState;

  // LCD Setup
  display.begin(Wire); //Set up the LCD for I2C communication

  display.setBacklight(255, 255, 255); //Set backlight to bright white
  display.setContrast(5); //Set contrast. Lower to 0 for higher contrast.

  display.clear(); //Clear the display - this moves the cursor to home position as well
  display.print("Joining network ...");


  // Wait for a USB serial connection for up to 15 seconds
  waitFor(Serial.isConnected, 15000);
}

int setScaleFactor(String factorString) {
  SCALE_FACTOR = atoi(factorString);
  
  return 0;
}

int setScaleOffset(String factorString) {
  SCALE_OFFSET = atoi(factorString);
  
  return 0;
}

boolean first = true;
void loop() {
  
  float nowTimeMillis = millis();  

  // First we read all inputs... even if these inputs aren't always needed for
  // our given state.. it's easier to just do it here as it costs very little.

  readScaleState(myScale, &scaleState);
  
  readUserInputState(isButtonPressedRightNow(), nowTimeMillis, &userInputState);

  // Determine next Gaggia state based on inputs and current state ...
  // (e.g. move to 'Done Brewing' state once target weight is achieved, etc.)
  GaggiaState nextGaggiaState = getNextGaggiaState(&currentGaggiaState, 
                                                   &heaterState, 
                                                   &scaleState, 
                                                   &userInputState,
                                                   &waterReservoirState,
                                                   &waterPumpState);

  if (first || nextGaggiaState.state != currentGaggiaState.state) {

    // Perform actions given current Gaggia state and input ...
    // This step does also mutate current state
    // (e.g. record weight of beans, tare measuring cup)

    // Things we do when we leave a state
    processOutgoingGaggiaState(&currentGaggiaState,
                               &nextGaggiaState,
                               &heaterState, 
                               &scaleState,
                               &displayState,
                               &waterReservoirState,
                               &waterPumpState,
                               nowTimeMillis);
  
      // Things we do when we enter a state
    processIncomingGaggiaState(&currentGaggiaState,
                               &nextGaggiaState,
                               &heaterState, 
                               &scaleState,
                               &displayState,
                               &waterReservoirState,
                               &waterPumpState,
                               nowTimeMillis);
  
    nextGaggiaState.stateEnterTimeMillis = millis();
  }

  currentGaggiaState = nextGaggiaState;

    // Things we do when we are within a state 
  processCurrentGaggiaState(&currentGaggiaState,
                            &heaterState, 
                            &scaleState,
                            &displayState,
                            &waterReservoirState,
                            &waterPumpState,
                            nowTimeMillis);

  if (isInTestMode) {
    delay(5000);
  } else {
    delay(50);
  }

  first = false;
}


// State transitions are documented here:
// https://docs.google.com/drawings/d/1EcaUzklpJn34cYeWsTnApoJhwBhA1Q4EVMr53Kz9T7I/edit?usp=sharing
GaggiaState getNextGaggiaState(GaggiaState *currentGaggiaState, 
                               HeaterState *heaterState,
                               ScaleState *scaleState,
                               UserInputState *userInputState,
                               WaterReservoirState *waterReservoirState,
                               WaterPumpState *waterPumpState) {

  if (manualNextGaggiaState.state != NA) {
    GaggiaState nextGaggiaState = manualNextGaggiaState;
    manualNextGaggiaState = naState;

    return nextGaggiaState;
  }

  switch (currentGaggiaState->state) {

    case HELLO :

      if (userInputState->state == SHORT_PRESS) {
        if (heaterState->measuredTemp >= TARGET_BREW_TEMP) {
          return coolStartState;
        } else {
          return tareCup1State;
        }
      }

      if (userInputState->state == LONG_PRESS) {
        return heatingToSteamState;
      }
      break;
      
      
    case TARE_CUP_1 :

      if (userInputState->state == SHORT_PRESS) {
        return measureBeansState;
      }
     
      if (userInputState->state == LONG_PRESS) {
        return helloState;
      }
      break;

    case MEASURE_BEANS :

      if (userInputState->state == SHORT_PRESS) {
        return tareCup2State;
      }
      if (userInputState->state == LONG_PRESS) {
        return helloState;
      }
      break;  

    case TARE_CUP_2 :

      if (userInputState->state == SHORT_PRESS) {
        return heatingState;
      }
      if (userInputState->state == LONG_PRESS) {
        return helloState;
      }
      break;

    case HEATING :

      if (heaterState->measuredTemp >= TARGET_BREW_TEMP) {
        return preinfusionState;
      }
      if (userInputState->state == LONG_PRESS) {
        return helloState;
      }
      break;

    case PREINFUSION :

      if ((millis() - currentGaggiaState->stateEnterTimeMillis) > 
             PREINFUSION_DURATION_SECONDS * 1000) {        
        return brewingState;
      }
      if (userInputState->state == LONG_PRESS) {
        return helloState;
      }
      break;

    case BREWING :

      if ((scaleState->measuredWeight - scaleState->tareWeight) >= 
            scaleState->targetWeight) {
        return doneBrewingState;
      }
      if (userInputState->state == LONG_PRESS) {
        return helloState;
      }
      break;

    case DONE_BREWING :

      if ((millis() - currentGaggiaState->stateEnterTimeMillis) > 
             DONE_BREWING_LINGER_TIME_SECONDS * 1000) {        
        return heatingToSteamState;
      }

      if (userInputState->state == LONG_PRESS) {
        return helloState;
      }
      break;

    case HEATING_TO_STEAM :

      if (heaterState->measuredTemp >= TARGET_STEAM_TEMP) {
        return steamingState;
      }
      if (userInputState->state == LONG_PRESS) {
        return helloState;
      }
      break;

    case STEAMING :

      if (userInputState->state == SHORT_PRESS) {
        return helloState;
      }
      if (userInputState->state == LONG_PRESS) {
        return helloState;
      }
      break;
      
    case COOL_START :

      if (userInputState->state == SHORT_PRESS) {
        return coolingState;
      }
      if (userInputState->state == LONG_PRESS) {
        return helloState;
      }
      break;

    case COOLING :

      if (userInputState->state == SHORT_PRESS) {
        return coolDoneState;
      }

      if (heaterState->measuredTemp < TARGET_BREW_TEMP) {
        return coolDoneState;
      }
      if (userInputState->state == LONG_PRESS) {
        return helloState;
      }
      break;

    case COOL_DONE :

      if (userInputState->state == SHORT_PRESS) {
        if (heaterState->measuredTemp >= TARGET_BREW_TEMP) {
          return coolStartState;
        } else {
          return helloState;
        }
      }
      if (userInputState->state == LONG_PRESS) {
        return helloState;
      }
      break;  
  } 

  if ((millis() - userInputState->lastUserInteractionTimeMillis) > 
    RETURN_TO_HOME_INACTIVITY_MINUTES * 60 * 1000) {
        return helloState;
  }

  return *currentGaggiaState;
}  

void processIncomingGaggiaState(GaggiaState *currentGaggiaState,  
                                GaggiaState *nextGaggiaState,
                                HeaterState *heaterState,
                                ScaleState *scaleState,
                                DisplayState *displayState,
                                WaterReservoirState *waterReservoirState,
                                WaterPumpState *waterPumpState,
                                float nowTimeMillis) {

  if (nextGaggiaState->state == PREINFUSION) {
    // at the moment, this is teh lowest value that produces
    // appreciable volume.. good for preinfusion
    
    // this needs to be done before we USE this value below when creating waterPumpPID!
    waterPumpState->targetPressureInBars = PRE_INFUSION_PSI;
  }

  if (nextGaggiaState->state == BREWING) {
    // this needs to be done before we USE this value below when creating waterPumpPID!
    waterPumpState->targetPressureInBars = DISPENSING_PSI;
  }

  if (nextGaggiaState->state == COOLING) {
    // this needs to be done before we USE this value below when creating waterPumpPID!
    waterPumpState->targetPressureInBars = DISPENSING_PSI;
  }

  // NOTE: This needs to be done AFTER we've calculated the proper targetPressure for 
  // current state!
  if (nextGaggiaState->dispenseWater) {
    publishParticleLog("dispense", "Launching Pressure PID");
    
    // The reason we do this here is because PID can't change its target value,
    // so we must create a new one when our target pressure changes..
    PID *thisWaterPumpPID = new PID(&waterPumpState->measuredPressureInBars, 
                                    &waterPumpState->pumpDutyCycle, 
                                    &waterPumpState->targetPressureInBars, 
                                    9.1, 0.3, 1.8, PID::DIRECT);
    
    // The Gaggia water pump doesn't energize at all below 30 duty cycle.
    thisWaterPumpPID->SetOutputLimits(30, 100);
    thisWaterPumpPID->SetMode(PID::AUTOMATIC);

    delete waterPumpState->waterPumpPID;
    waterPumpState->waterPumpPID = thisWaterPumpPID;
  }

  if (nextGaggiaState->brewHeaterOn) {
    // The reason we do this here is because PID can't change its target value,
    // so we must create a new one when our target temperature changes..
    PID *thisHeaterPID = new PID(&heaterState->measuredTemp, 
                                 &heaterState->heaterDurationMillis, 
                                 &TARGET_BREW_TEMP, 
                                 9.1, 0.3, 1.8, PID::DIRECT);
    thisHeaterPID->SetOutputLimits(0, 250);
    thisHeaterPID->SetMode(PID::AUTOMATIC);

    delete heaterState->heaterPID;
    heaterState->heaterPID = thisHeaterPID;
  }

  if (nextGaggiaState->steamHeaterOn) {
    PID *thisHeaterPID = new PID(&heaterState->measuredTemp, 
                                 &heaterState->heaterDurationMillis, 
                                 &TARGET_STEAM_TEMP, 
                                 9.1, 0.3, 1.8, PID::DIRECT);
    thisHeaterPID->SetOutputLimits(0, 250);
    thisHeaterPID->SetMode(PID::AUTOMATIC);

    delete heaterState->heaterPID;
    heaterState->heaterPID = thisHeaterPID;
  }
}


void processOutgoingGaggiaState(GaggiaState *currentGaggiaState,  
                                GaggiaState *nextGaggiaState,
                                HeaterState *heaterState,
                                ScaleState *scaleState,
                                DisplayState *displayState,
                                WaterReservoirState *waterReservoirState,
                                WaterPumpState *waterPumpState,
                                float nowTimeMillis) {

  // Process Tare Scale
  if (currentGaggiaState->tareScale == true) {
    scaleState->tareWeight = scaleState->measuredWeight;
  }

  // Process Record Weight 
  if (currentGaggiaState->recordWeight) {
    scaleState->targetWeight = 
      (scaleState->measuredWeight - scaleState->tareWeight)*BREW_WEIGHT_TO_BEAN_RATIO; 
  }

  // this is useful for state metrics...
  float timeSpentInCurrentStateMillis = millis() - currentGaggiaState->stateEnterTimeMillis;
  

  if (currentGaggiaState->state == BREWING) {
    // Just finished brewing.. Need to calculate flow rate...

    // The rate is usually expressed as grams/30seconds
    float timeSpent30SecondIntervals = timeSpentInCurrentStateMillis/(30 * 1000.0);

    float dispendedWaterWeight =  scaleState->targetWeight;

    publishParticleLog("flowRate", "(" + String(dispendedWaterWeight) + "/" + String(timeSpent30SecondIntervals) + ")");

    scaleState->flowRate = dispendedWaterWeight/timeSpent30SecondIntervals;
  }
}

void processCurrentGaggiaState(GaggiaState *currentGaggiaState,  
                               HeaterState *heaterState,
                               ScaleState *scaleState,
                               DisplayState *displayState,
                               WaterReservoirState *waterReservoirState,
                               WaterPumpState *waterPumpState,
                               float nowTimeMillis) {

  // 
  // Process Display
  //
  displayState->display1 = updateDisplayLine(currentGaggiaState->display1, 
                                             1, 
                                             heaterState, 
                                             scaleState, 
                                             waterPumpState,
                                             displayState->display1);

  displayState->display2 = updateDisplayLine(currentGaggiaState->display2, 
                                             2, 
                                             heaterState, 
                                             scaleState, 
                                             waterPumpState,
                                             displayState->display2);

  displayState->display3 = updateDisplayLine(currentGaggiaState->display3, 
                                             3, 
                                             heaterState, 
                                             scaleState,  
                                             waterPumpState,
                                             displayState->display3);

  displayState->display4 = updateDisplayLine(currentGaggiaState->display4, 
                                             4, 
                                             heaterState, 
                                             scaleState, 
                                             waterPumpState,
                                             displayState->display4);



  // Process Heaters
  //
  // Even though a heater may be 'on' during this state, the control system
  // for the heater turns it off and on intermittently in an attempt to regulate
  // the temperature around the target temp.
  if (currentGaggiaState->brewHeaterOn) {
    readHeaterState(MAX6675_CS_brew, MAX6675_SO_brew, MAX6675_SCK, heaterState);  

    if (shouldTurnOnHeater(nowTimeMillis, heaterState)) {
      turnHeaterOn();
    } else {
      turnHeaterOff();
    } 
  }
  if (currentGaggiaState->steamHeaterOn) {
    readHeaterState(MAX6675_CS_steam, MAX6675_SO_steam, MAX6675_SCK, heaterState); 

    if (shouldTurnOnHeater(nowTimeMillis, heaterState)) {
      turnHeaterOn();
    } else {
      turnHeaterOff();
    } 
  }
  if (!currentGaggiaState->brewHeaterOn && !currentGaggiaState->steamHeaterOn) {
      turnHeaterOff();
  }

  // Process Dispense Water 
  if (currentGaggiaState->dispenseWater) {
    dispenseWater();
  } else {
    stopDispensingWater();
  }

  if (currentGaggiaState->fillingReservoir) {

    if (doesWaterReservoirNeedFilling(DISPENSE_WATER,  WATER_RESERVOIR_SENSOR, waterReservoirState)) {
      waterReservoirState->isSolenoidOn = true;
      turnWaterReservoirSolenoidOn();
    } else {
      waterReservoirState->isSolenoidOn = false;
      turnWaterReservoirSolenoidOff();
    }

  } else {
    turnWaterReservoirSolenoidOff();
  }

  if (currentGaggiaState->measureTemp) {
    readHeaterState(MAX6675_CS_brew, MAX6675_SO_brew, MAX6675_SCK, heaterState);  
  }
}

void readScaleState(NAU7802 myScale, ScaleState *scaleState) {
  scaleState->measuredWeight = 0;

  if (myScale.available() == true) {
    float reading = myScale.getReading();
  
     //Log.error("rawReading: " + String(reading));

    float scaledReading = ((reading * SCALE_FACTOR) - SCALE_OFFSET)/10000000.0;

    //Log.error("scaledReading: " + String(scaledReading));

    scaleState->measuredWeight = scaledReading;
  }
}

boolean isButtonPressedRightNow() {
  return userButton.isPressed();
}

void readUserInputState(boolean isButtonPressedRightNow, float nowTimeMillis, UserInputState *userInputState) {
  // Derive current user input state

  if (isButtonPressedRightNow) {
    if (userInputState->buttonPressStartTimeMillis < 0) {  
      // rising edge detected...
      userInputState->buttonPressStartTimeMillis = nowTimeMillis;
    }
  
    // if long press hasn't already been identified (parking on button), and we identify a LONG press...
    if ((userInputState->buttonPressStartTimeMillis != 0) && 
        (nowTimeMillis - userInputState->buttonPressStartTimeMillis > BUTTON_LONG_PRESS_DURATION_MILLIS)) {
      userInputState->state = LONG_PRESS;
      userInputState->buttonPressStartTimeMillis = 0; // this indicates a LONG press was already identified..
      userInputState->lastUserInteractionTimeMillis = nowTimeMillis;

      return;
    }
  } else {
    if (userInputState->buttonPressStartTimeMillis > 0) {  
      // falling edge detected...
      if (nowTimeMillis - userInputState->buttonPressStartTimeMillis < BUTTON_LONG_PRESS_DURATION_MILLIS) {
        userInputState->state = SHORT_PRESS;
        userInputState->buttonPressStartTimeMillis = -1;
        userInputState->lastUserInteractionTimeMillis = nowTimeMillis;

        return;
      }
    } else {
      // button is not being pressed
      
      // we do this because we are in a 'button idle' state and we want to ensure this 
      // is reset.. it could have been @ -999 due to a extended LONG press
      userInputState->buttonPressStartTimeMillis = -1;
    }
  }

  // if here, fallback is IDLE
  userInputState->state = IDLE;
}

void readHeaterState(int CHIP_SELECT_PIN, int SERIAL_OUT_PIN, int SERIAL_CLOCK_PIN, HeaterState *heaterState) {

  uint16_t measuredValue;

  // enable MAX6675
  digitalWrite(CHIP_SELECT_PIN, LOW);
  delay(1);

  // Read in 16 bits,
  //  15    = 0 always
  //  14..2 = 0.25 degree counts MSB First
  //  2     = 1 if thermocouple is open circuit  
  //  1..0  = uninteresting status
  
  measuredValue = shiftIn(SERIAL_OUT_PIN, SERIAL_CLOCK_PIN, MSBFIRST);
  measuredValue <<= 8;
  measuredValue |= shiftIn(SERIAL_OUT_PIN, SERIAL_CLOCK_PIN, MSBFIRST);
  
  // disable MAX6675
  digitalWrite(CHIP_SELECT_PIN, HIGH);

  if (measuredValue & 0x4) {    
    // Bit 2 indicates if the thermocouple is disconnected
    Log.error("Thermocouple is disconnected!");
    heaterState->thermocoupleError = true;

  } else {
    
    heaterState->thermocoupleError = false;

    // The lower three bits (0,1,2) are discarded status bits
    measuredValue >>= 3;

    publishParticleLog("renderHeaterState", "measuredInC:" + String(measuredValue*0.25));

    // The remaining bits are the number of 0.25 degree (C) counts
    heaterState->measuredTemp = measuredValue*0.25;
  }
}

void turnHeaterOn() {
  publishParticleLog("heater", "on");
  digitalWrite(HEATER, HIGH);
}

boolean isHeaterOn() {
  return digitalRead(HEATER) == HIGH;
}


void turnHeaterOff() {
  publishParticleLog("heater", "off");
  digitalWrite(HEATER, LOW);
}

void dispenseWater() {
  publishParticleLog("dispenser", "dispensingOn");

  readPumpState(&waterPumpState);  

  // This triggers the PID to use previous and current
  // measured values to calculate the current required
  // dutyCycle...
  waterPumpState.waterPumpPID->Compute();
  publishParticleLog("dispenser", "pumpDutyCycle: " + String(waterPumpState.pumpDutyCycle));

  // The zero crossings from the incoming AC sinewave will trigger
  // this interrupt handler, which will modulate the power duty cycle to
  // the water pump..
  //
  // This will not trigger unless water dispenser is running.
  //
  attachInterrupt(ZERO_CROSS_DISPENSE_POT, handleZeroCrossingInterrupt, RISING, 0);

  digitalWrite(DISPENSE_WATER, HIGH);
}

void readPumpState(WaterPumpState *waterPumpState) {
  // reading from first channel of the 1015

  // no pressure ~1100
  int rawPressure = ads1115.readADC_SingleEnded(0);
  int normalizedPressureInBars = (rawPressure-PRESSURE_SENSOR_OFFSET)/PRESSURE_SENSOR_SCALE_FACTOR;

  publishParticleLog("pump", "rawPressure: " + String(rawPressure) + "', normalizedPressure: " + String(normalizedPressureInBars));

  waterPumpState->measuredPressureInBars = normalizedPressureInBars;

  // NJD TODO PRESSURE  - this is all we have to switch when we get the pressure sensor installed.
  // If we say it returns 0, then the pump will run @ 100% duty cycle always. which is ok for now.
  //waterPumpState->measuredPressureInBars = 0;
  
}

void stopDispensingWater() {
  publishParticleLog("dispenser", "dispensingOff");

  detachInterrupt(ZERO_CROSS_DISPENSE_POT);

  digitalWrite(DISPENSE_WATER, LOW);
  digitalWrite(DISPENSE_POT, LOW);
}

void turnWaterReservoirSolenoidOn() {
  publishParticleLog("waterReservoirSolenoid", "on");
  digitalWrite(WATER_RESERVOIR_SOLENOID, HIGH);
}

void turnWaterReservoirSolenoidOff() {
  publishParticleLog("waterReservoirSolenoid", "off");
  digitalWrite(WATER_RESERVOIR_SOLENOID, LOW);
}

// If no decoding is necessary, the original string
// is returned
String decodeMessageIfNecessary(char* _message, 
                                char* escapeSequence,
                                long firstValue,
                                long secondValue,                         
                                char* units) {
  char *message = _message; 

  if (strcmp(message, escapeSequence) == 0) {
    char firstValueString[9];
    sprintf(firstValueString, "%ld", firstValue);

    char *decodedMessage;
    char lineBuff[20] = "                   ";
    if (secondValue != 0) {
      char secondValueString[9];
      sprintf(secondValueString, "%ld", secondValue);

      decodedMessage = strcat(strcat(strcat(firstValueString, "/"), secondValueString), units);
    } else {
      decodedMessage = strcat(firstValueString, units);
    }

    return String((char *)(memcpy(lineBuff, decodedMessage, strlen(decodedMessage))));
  }

  return String("");
}

// line starts at 1
String updateDisplayLine(char *message, 
                        int line,
                        HeaterState *heaterState,
                        ScaleState *scaleState,
                        WaterPumpState *waterPumpState,
                        String previousLineDisplayed) {

  display.setCursor(0,line-1);
  
  String lineToDisplay; 
  
  lineToDisplay = decodeMessageIfNecessary(message,
                                           "{adjustedWeight}/{targetBeanWeight}",
                                           filterLowWeight(scaleState->measuredWeight - scaleState->tareWeight),
                                           TARGET_BEAN_WEIGHT,
                                           " g");
  if (lineToDisplay == "") {
    lineToDisplay = decodeMessageIfNecessary(message,
                                            "{measuredBrewTemp}/{targetBrewTemp}",
                                            heaterState->measuredTemp,
                                            TARGET_BREW_TEMP,
                                            " degrees C");
    if (lineToDisplay == "") {
      lineToDisplay = decodeMessageIfNecessary(message,
                                              "{adjustedWeight}/{targetBrewWeight}",
                                              filterLowWeight(scaleState->measuredWeight - scaleState->tareWeight),
                                              scaleState->targetWeight,
                                              " g");
      if (lineToDisplay == "") {
        lineToDisplay = decodeMessageIfNecessary(message,
                                              "{measuredSteamTemp}/{targetSteamTemp}",
                                              heaterState->measuredTemp,
                                              TARGET_STEAM_TEMP,
                                              " degrees C");
        if (lineToDisplay == "") {
          lineToDisplay = decodeMessageIfNecessary(message,
                                                "{measuredWeight}",
                                                filterLowWeight(scaleState->measuredWeight - scaleState->tareWeight),
                                                0,
                                                " g");
          if (lineToDisplay == "") {
            lineToDisplay = decodeMessageIfNecessary(message,
                                                     "{flowRate}",
                                                     scaleState->flowRate,
                                                     0,
                                                     " g/30sec");
            if (lineToDisplay == "") {
              lineToDisplay = decodeMessageIfNecessary(message,
                                                       "{measuredBars}/{targetBars}",
                                                       waterPumpState->measuredPressureInBars,
                                                       waterPumpState->targetPressureInBars,
                                                       " bars");
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

long filterLowWeight(long weight) {
  if (abs(weight) < LOW_WEIGHT_THRESHOLD) {
    return 0;
  } else {
    return weight;
  }
}

boolean shouldTurnOnHeater(float nowTimeMillis,
                           HeaterState *heaterState) {
                        
  boolean shouldTurnOnHeater = false;

  if (heaterState->heaterStarTime > 0 ) {
    // we're in a heat cycle....

    if ((nowTimeMillis - heaterState->heaterStarTime) > heaterState->heaterDurationMillis) {
      // done this heat cycle!

      heaterState->heaterStarTime = -1;
      publishParticleLog("shouldTurnOnHeater", "doneCycle..off");
      shouldTurnOnHeater = false;
    } else {
      shouldTurnOnHeater = true;
    }
  } else {
    // determine if we should be in a heat cycle ...

    // This triggers the PID to use previous and current
    // measured values to calculate the current required
    // heater interval ...
    // The output of this calculation is a new 'heaterDurationMillis' value.
    heaterState->heaterDurationMillis = 0;
    heaterState->heaterPID->Compute();

    if (heaterState->heaterDurationMillis > 0) {
      publishParticleLog("shouldTurnOnHeater", 
        "startCycle..on, with duration:" + String(heaterState->heaterDurationMillis));
      heaterState->heaterStarTime = nowTimeMillis;

      shouldTurnOnHeater = true;
    }
  }

  return shouldTurnOnHeater;
}


boolean doesWaterReservoirNeedFilling(int CHIP_ENABLE_PIN, int ANALOG_INPUT_PIN, WaterReservoirState *waterReservoirState) {

  if (digitalRead(CHIP_ENABLE_PIN) == LOW) {
    digitalWrite(CHIP_ENABLE_PIN, HIGH);
  }

  int measuredWaterLevel = analogRead(ANALOG_INPUT_PIN);
  
  waterReservoirState->measuredWaterLevel = measuredWaterLevel;

  // Schmitt Trigger
  if (waterReservoirState->isSolenoidOn) {
    if (measuredWaterLevel < HIGH_WATER_RESEVOIR_LIMIT) {
      return true;
    }
  } else {
    if (measuredWaterLevel < LOW_WATER_RESEVOIR_LIMIT) {
      return true;
    }
  }

  if (digitalRead(CHIP_ENABLE_PIN) == HIGH) {
    digitalWrite(CHIP_ENABLE_PIN, LOW);
  }

  return false;
}

void publishParticleLog(String group, String message) {
    if (PLATFORM_ID == PLATFORM_ARGON && isInTestMode) {
        Particle.publish(group, message, 60, PUBLIC);
    }
}

int turnOnTestMode(String _na) {

    Particle.publish("config", "testMode turned ON", 60, PUBLIC);
    isInTestMode = true;

    return 1;
}

int turnOffTestMode(String _na) {

    Particle.publish("config", "testMode turned OFF", 60, PUBLIC);
    isInTestMode = false;

    return 1;
}

int _setBrewingState(String _) {
  
  // by jumping to this state, we are bypassing earlier states.. so we need to
  // fake any data that should have been collected.
  scaleState.tareWeight = 320;
  scaleState.targetWeight = 100;

  manualNextGaggiaState = brewingState;
  return 1;
}

int _setHeatingState(String _) {
  
  // by jumping to this state, we are bypassing earlier states.. so we need to
  // fake any data that should have been collected.
  scaleState.tareWeight = 320;
  scaleState.targetWeight = 100;

  manualNextGaggiaState = heatingState;
  return 1;
}

int _setCoolingState(String _) {
  manualNextGaggiaState = coolingState;
  return 1;
}

int _setHelloState(String _) {
  manualNextGaggiaState = helloState;
  return 1;
}

// Make sure you are dispensing water when you call this! The power from
// the dispenser is used to power this sensor
int _readWaterLevel() {
  
  if (digitalRead(DISPENSE_WATER) == LOW) {
    digitalWrite(DISPENSE_WATER, HIGH);
  }

  int measuredWaterLevel = analogRead(WATER_RESERVOIR_SENSOR);

  if (digitalRead(DISPENSE_WATER) == HIGH) {
    digitalWrite(DISPENSE_WATER, LOW);
  }

  return measuredWaterLevel;
}

String _readCurrentState() {
  return String(currentGaggiaState.state);
}

String _targetPressureInBars() {
  return String(waterPumpState.targetPressureInBars);
}

String _measuredPressureInBars() {

  readPumpState(&waterPumpState);  

  return String(waterPumpState.measuredPressureInBars);
}

// This is an Interrupt Service Routine (ISR) so it cannot take any arguments
// nor return any values.
//
// We consider our current
void turnOnDispensePotISR() {
  digitalWrite(DISPENSE_POT, HIGH);
}
Timer timer(0, turnOnDispensePotISR, true);
void handleZeroCrossingInterrupt() {
  // IMPORTANT!: the Triac will automatically shut off at every zero
  // crossing.. so it's best to start the cycle with teh triac off and
  // then turn it on for the remainder of the cycle  

  digitalWrite(DISPENSE_POT, LOW);
  delayMicroseconds(10);
  
  // We assume 60Hz cycle (1000ms/60 = 16ms per cycle.)
  // If we wait 8ms before turning on, the next cycle will happen immediately
  // and shut off.. so 8ms is essentially 0% duty cycle
  // If we wait 4ms before turning on, that's half the cycle.. so that's 50%
  // If we wait 0ms before turning on, that's 100% duty cycle.

  // ok for some reason, only values of 40 and 100 work

  // NOTE: dutyCycle below 30 is kinda useless.. doesn't really energize water
  // pump
  int offIntervalMs = 8 - round((8*(waterPumpState.pumpDutyCycle/100.0)));

  // There needs to be a minimal delay between LOW and HIGH for the TRIAC
  offIntervalMs = max(offIntervalMs, 1);

  // 'FromISR' is critical otherwise, the timer hangs the ISR.
  timer.changePeriodFromISR(offIntervalMs);
  timer.startFromISR();  
}
