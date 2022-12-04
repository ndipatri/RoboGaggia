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
#include <Adafruit_IO_Client.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_SPARK.h"


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

// This sends water to the group head
#define SOLENOID_VALVE_SSR  TX

// 1 - air , 0 - water
// This sensor draws max 2.5mA so it can be directly powered by 3.3v on
// Argon
#define WATER_RESERVOIR_SENSOR  A1

// send high to turn on solenoid
#define WATER_RESERVOIR_SOLENOID  RX 

#define SCALE_SAMPLE_SIZE 10 
#define LOOP_INTERVAL_MILLIS 50 


//
// Constants
//

// *************
// FEATURE FLAGS 
// *************

boolean TELEMETRY_ENABLED = false;

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

// y = 6.33E-04*x + -13.2
double SCALE_FACTOR = 0.000633; // 3137;  
double SCALE_OFFSET = -13.2;  // 47;

// Below which we consider weight to be 0
int LOW_WEIGHT_THRESHOLD = 5;

int RETURN_TO_HOME_INACTIVITY_MINUTES = 10;

// How long we will the portafilter with semi-hot water
int PREINFUSION_DURATION_SECONDS = 5;
  
int DONE_BREWING_LINGER_TIME_SECONDS = 5;
  
// How many 60Hz cycles we consider in our flow rate
// So 10 means we count 10 cycles before we reset our count.  We could
// then desire a 50% duty cycle, which would mean 5 cycles on and 
// 5 cycles off
int DISPENSE_FLOW_RATE_TOTAL_CYCES = 20;

// The Gaggia currently has a 12bar spring in its 
// OPV (over pressure valve)
double DISPENSING_BAR = 6.0;
double PRE_INFUSION_BAR = 1.0;
double BACKFLUSH_BAR = 4.0;

// These were emperically derived.  They are highly dependent on the actual system , but should now work
// for any RoboGaggia.
// see https://en.wikipedia.org/wiki/PID_controller#Loop_tuning
double PID_kP = 20.0;
double PID_kI = 5.0;
double PID_kD = 2.0;

// see https://docs.google.com/spreadsheets/d/1_15rEy-WI82vABUwQZRAxucncsh84hbYKb2WIA9cnOU/edit?usp=sharing
// as shown in shart above, the following values were derived by hooking up a bicycle pump w/ guage to the
// pressure sensor and measuring a series of values vs bar pressure. 
double PRESSURE_SENSOR_SCALE_FACTOR = 1658.0;
int PRESSURE_SENSOR_OFFSET = 2470; 

int MIN_PUMP_DUTY_CYCLE = 30;
int MAX_PUMP_DUTY_CYCLE = 100;

int NUMBER_OF_CLEAN_CYCLES = 20; // (10 on and off based on https://youtu.be/lfJgabTJ-bM?t=38)
int SECONDS_PER_CLEAN_CYCLE = 4; 

int TELEMETRY_PERIOD_MILLIS = 1000; 

//
// State
//

struct WaterPumpState {

double targetPressureInBars = PRE_INFUSION_BAR; 

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

struct NetworkState {
  boolean connected = false;
} networkState;


struct WaterReservoirState {

  boolean isSolenoidOn = false;

} waterReservoirState;
boolean doesWaterReservoirNeedFilling();


struct ScaleState {

  // The current weight measurement is a sliding average
  float avgWeights[SCALE_SAMPLE_SIZE];
  byte avgWeightIndex = 0;

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
  double heaterShouldBeOn;

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


enum GaggiaStateEnum {
  HELLO = 0, 
  FEATURES = 22, 
  TARE_CUP_BEFORE_MEASURE = 1, 
  MEASURE_BEANS = 2,  
  TARE_CUP_AFTER_MEASURE = 3, 
  HEATING_TO_BREW = 4, 
  PREINFUSION = 5, 
  BREWING = 6, 
  DONE_BREWING = 7, 
  HEATING_TO_STEAM = 8, 
  STEAMING = 9, 
  COOL_START = 10, 
  COOLING = 11, 
  COOL_DONE = 12, 
  CLEAN_INSTRUCTION_1 = 13, 
  CLEAN_INSTRUCTION_2 = 14, 
  CLEAN_SOAP = 15, 
  CLEAN_INSTRUCTION_3 = 16, 
  CLEAN_RINSE = 17, 
  CLEAN_DONE = 18, 
  HEATING_TO_DISPENSE = 23, 
  DISPENSE_HOT_WATER = 21, 
  IGNORING_NETWORK = 19, 
  JOINING_NETWORK = 20, 
  NA = 24  // indicates developer is NOT explicitly setting a test state through web interface
};
struct GaggiaState {
   enum GaggiaStateEnum state;   
   char *display1 = "";
   char *display2 = "";
   char *display3 = "";
   char *display4 = "";
   boolean  waterReservoirSolenoidOn = false;
   
   // ************
   // WARNING: this cannot be done at the same time (or even in an adjacent state)
   // to dispensing water.
   // ************
   boolean  fillingReservoir = false;

   boolean  brewHeaterOn = false;
   boolean  steamHeaterOn = false;
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

   float nextTelemetryMillis = -1;
} 
helloState,
featuresState,
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


struct DisplayState {
  String display1;
  String display2;
  String display3;
  String display4;

} displayState;
String updateDisplayLine(char *message, 
                        int line,
                        GaggiaState *gaggiaState,
                        HeaterState *heaterState,
                        ScaleState *scaleState,
                        WaterPumpState *waterPumpState,
                        NetworkState *networkState,
                        String previousLineDisplayed);
SerLCD display; // Initialize the library with default I2C address 0x72



boolean isInTestMode = false;

// It is possible to manually direct the next station transition
GaggiaState manualNextGaggiaState;

// Using all current state, we derive the next state of the system
GaggiaState getNextGaggiaState(GaggiaState *currentGaggiaState, 
                               HeaterState *heaterState,
                               ScaleState *scaleState,
                               UserInputState *userInputState,
                               WaterReservoirState *waterReservoirState,
                               WaterPumpState *waterPumpState,
                               NetworkState *networkState);

// Once we know the state, we affect appropriate change in our
// system attributes (e.g. temp, display)
// Things we do while we are in a state 
void processCurrentGaggiaState(GaggiaState *currentGaggiaState,
                               HeaterState *heaterState,
                               ScaleState *scaleState,
                               DisplayState *displayState,
                               WaterReservoirState *waterReservoirState,
                               WaterPumpState *waterPumpState,
                               NetworkState *networkState,
                               float nowTimeMillis);
// Things we do when we leave a state
void processOutgoingGaggiaState(GaggiaState *currentGaggiaState,
                                GaggiaState *nextGaggiaState, 
                                HeaterState *heaterState,
                                ScaleState *scaleState,
                                DisplayState *displayState,
                                WaterReservoirState *waterReservoirState,
                                WaterPumpState *waterPumpState,
                                NetworkState *networkState,
                                float nowTimeMillis);
// Things we do when we enter a state
void processIncomingGaggiaState(GaggiaState *currentGaggiaState,
                                GaggiaState *nextGaggiaState, 
                                HeaterState *heaterState,
                                ScaleState *scaleState,
                                DisplayState *displayState,
                                WaterReservoirState *waterReservoirState,
                                WaterPumpState *waterPumpState,
                                NetworkState *networkState,
                                float nowTimeMillis);
GaggiaState returnToHelloState(ScaleState *scaleState);
float timeSpentInCurrentStateMillis(GaggiaState *currentGaggiaState);
void sendTelemetryIfNecessary(float nowTimeMillis,
                              GaggiaState *gaggiaState,
                              HeaterState *heaterState,
                              ScaleState *scaleState,
                              WaterReservoirState *waterReservoirState,
                              WaterPumpState *waterPumpState,
                              NetworkState *networkState); 
char* getStateName(GaggiaStateEnum stateEnum);
void sendMessageToCloud(const char* message, Adafruit_MQTT_Publish* topic, NetworkState *networkState);

// If you check in this code WITH this KEY defined, it will be detected by IO.Adafruit
// and IT WILL BE DISABLED !!!  So please delete value below before checking in!
// ***************** !!!!!!!!!!!!!! **********
#define AIO_KEY         "XXX" // Adafruit IO AIO Key
#define AIO_SERVER      "io.adafruit.com"
#define AIO_SERVERPORT  1883                   // use 8883 for SSL
String AIO_USERNAME     = "ndipatri";
// ***************** !!!!!!!!!!!!!! **********

TCPClient client; // TCP Client used by Adafruit IO library

Adafruit_MQTT_SPARK mqtt(&client, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_KEY);

String mqttRoboGaggiaTelemetryTopicName = AIO_USERNAME + "/feeds/roboGaggiaTelemetry"; 
Adafruit_MQTT_Publish mqttRoboGaggiaTelemetryTopic = Adafruit_MQTT_Publish(&mqtt,  mqttRoboGaggiaTelemetryTopicName);

Adafruit_MQTT_Subscribe errors = Adafruit_MQTT_Subscribe(&mqtt, AIO_USERNAME + "/errors");
Adafruit_MQTT_Subscribe throttle = Adafruit_MQTT_Subscribe(&mqtt, AIO_USERNAME + "/throttle");

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
  pinMode(SOLENOID_VALVE_SSR, OUTPUT);
  pinMode(WATER_RESERVOIR_SOLENOID, OUTPUT);

  // water reservoir sensor
  pinMode(WATER_RESERVOIR_SENSOR, INPUT);

  // // Useful state exposed to Particle web console
  Particle.variable("heaterTempC", heaterState.measuredTemp);
  Particle.variable("targetBrewTempC", TARGET_BREW_TEMP);
  Particle.variable("thermocoupleError",  heaterState.thermocoupleError);
  Particle.variable("tareWeightGrams",  scaleState.tareWeight);
  Particle.variable("measuredWeightGrams",  scaleState.measuredWeight);
  Particle.variable("targetPressureInBars", _targetPressureInBars);
  Particle.variable("isInTestMode",  isInTestMode);
  Particle.variable("doesWaterNeedFilling",  doesWaterReservoirNeedFilling);
  Particle.variable("currentState", _readCurrentState);
  Particle.variable("PID_kP", PID_kP);
  Particle.variable("PID_kI", PID_kI);
  Particle.variable("PID_kD", PID_kD);
  Particle.variable("DispensingBar", DISPENSING_BAR);
  Particle.variable("PreInfusionBar", PRE_INFUSION_BAR);

  Particle.function("turnOnTestMode", turnOnTestMode);
  Particle.function("turnOffTestMode", turnOffTestMode);

  Particle.function("setHeatingState", _setHeatingState);
  Particle.function("setBrewingState", _setBrewingState);
  Particle.function("setSteamingState", _setSteamingState);
  Particle.function("setCoolingState", _setCoolingState);
  Particle.function("setHelloState", _setHelloState);
  Particle.function("setDispenseHotWater", _setDispenseHotWater);

  // Can't expose all functions at once...
  //Particle.function("set_kP", _setPID_kP);
  //Particle.function("set_kI", _setPID_kI);
  //Particle.function("set_kD", _setPID_kD);

  //Particle.function("setScaleFactor", setScaleFactor);
  //Particle.function("setScaleOffset", setScaleOffset);

  // Define all possible states of RoboGaggia
  helloState.state = HELLO; 
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
  featuresState.display2 =            "Click for hot water ";
  featuresState.display3 =            "   through wand,    ";
  featuresState.display4 =            "Hold for Clean      ";

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
  heatingToDispenseState.display3 =   "{measuredSteamTemp}/{targetSteamTemp}";
  heatingToDispenseState.display4 =    "Please wait ...     ";
  heatingToDispenseState.steamHeaterOn = true; 

  dispenseHotWaterState.state = DISPENSE_HOT_WATER; 
  dispenseHotWaterState.display1 =          "Use steam wand to  ";
  dispenseHotWaterState.display2 =          "dispense hot water. ";
  dispenseHotWaterState.display3 =          "                    ";
  dispenseHotWaterState.display4 =          "Click when Done     ";
  dispenseHotWaterState.steamHeaterOn = true; 
  dispenseHotWaterState.waterThroughWand = true; 

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

  // LCD Setup
  display.begin(Wire); //Set up the LCD for I2C communication

  display.setBacklight(255, 255, 255); //Set backlight to bright white
  display.setContrast(5); //Set contrast. Lower to 0 for higher contrast.

  display.clear(); //Clear the display - this moves the cursor to home position as well

  myScale.setGain(NAU7802_GAIN_64); //Gain can be set to 1, 2, 4, 8, 16, 32, 64, or 128. default is 16
  myScale.setSampleRate(NAU7802_SPS_80); //Sample rate can be set to 10, 20, 40, 80, or 320Hz. default is 10
  myScale.calibrateAFE(); //Does an internal calibration. Recommended after power up, gain changes, sample rate changes, or channel changes.

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

char* getStateName(GaggiaStateEnum stateEnum) {
   switch (stateEnum) {
    case HELLO: return "hello";
    case FEATURES: return "features";
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

boolean first = true;
void loop() {
  
  float nowTimeMillis = millis();  

  readScaleState(myScale, &scaleState);
  
  readUserInputState(isButtonPressedRightNow(), nowTimeMillis, &userInputState);

  // Determine next Gaggia state based on inputs and current state ...
  // (e.g. move to 'Done Brewing' state once target weight is achieved, etc.)
  GaggiaState nextGaggiaState = getNextGaggiaState(&currentGaggiaState, 
                                                   &heaterState, 
                                                   &scaleState, 
                                                   &userInputState,
                                                   &waterReservoirState,
                                                   &waterPumpState,
                                                   &networkState);

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
                               &networkState,
                               nowTimeMillis);
  
      // Things we do when we enter a state
    processIncomingGaggiaState(&currentGaggiaState,
                               &nextGaggiaState,
                               &heaterState, 
                               &scaleState,
                               &displayState,
                               &waterReservoirState,
                               &waterPumpState,
                               &networkState,
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
                            &networkState,
                            nowTimeMillis);

  sendTelemetryIfNecessary(nowTimeMillis,
                           &currentGaggiaState,
                           &heaterState,
                           &scaleState,
                           &waterReservoirState,
                           &waterPumpState,
                           &networkState);
  
    // resume service loop
    if (networkState.connected) {
      Particle.process();
    }

  if (isInTestMode) {
    delay(2000);
  } else {
    delay(LOOP_INTERVAL_MILLIS);
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
                               WaterPumpState *waterPumpState,
                               NetworkState *networkState) {

  if (manualNextGaggiaState.state != NA) {
    GaggiaState nextGaggiaState = manualNextGaggiaState;
    manualNextGaggiaState = naState;

    return nextGaggiaState;
  }

  switch (currentGaggiaState->state) {

    case IGNORING_NETWORK :

      if (WiFi.isOff()) {
        return helloState;
      }

      break;

    case JOINING_NETWORK :

      if (networkState->connected) {
        return helloState;
      }
      
      if (userInputState->state == SHORT_PRESS || userInputState->state == LONG_PRESS) {
        return ignoringNetwork;
      }

      break;

    case HELLO :

      // We have to give the scale long enough to tare proper weight
      if (userInputState->state == SHORT_PRESS) {
        if (heaterState->measuredTemp >= TARGET_BREW_TEMP * 1.20) {
          return coolStartState;
        } else {
          return tareCup1State;
        }
      }

      if (userInputState->state == LONG_PRESS) {
        return featuresState;
      }
      break;

    case FEATURES :

      if (userInputState->state == SHORT_PRESS) {
        return heatingToDispenseState;
      }
     
      if (userInputState->state == LONG_PRESS) {
        return cleanInstructions1State;
      }
      break;      
      
    case TARE_CUP_BEFORE_MEASURE :

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

    case TARE_CUP_AFTER_MEASURE :

      if (userInputState->state == SHORT_PRESS) {
        return heatingToBrewState;
      }
      if (userInputState->state == LONG_PRESS) {
        return helloState;
      }
      break;

    case HEATING_TO_BREW :

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
      
    case HEATING_TO_DISPENSE :

      if (heaterState->measuredTemp >= TARGET_STEAM_TEMP) {
        return dispenseHotWaterState;
      }
     
      if (userInputState->state == LONG_PRESS) {
        return helloState;
      }
      break; 

    case DISPENSE_HOT_WATER :

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
  
    case CLEAN_INSTRUCTION_1 :

      if (userInputState->state == SHORT_PRESS) {
        return cleanInstructions2State;
      }
     
      if (userInputState->state == LONG_PRESS) {
        return helloState;
      }
      break;

    case CLEAN_INSTRUCTION_2 :

      if (userInputState->state == SHORT_PRESS) {
        return cleanCycle1State;
      }
     
      if (userInputState->state == LONG_PRESS) {
        return helloState;
      }
      break;

    case CLEAN_SOAP :

      if (currentGaggiaState->counter == currentGaggiaState->targetCounter-1) {
        return cleanInstructions3State;
      }
     
      if (userInputState->state == LONG_PRESS) {
        return helloState;
      }
      break;

    case CLEAN_INSTRUCTION_3 :

      if (userInputState->state == SHORT_PRESS) {
        return cleanCycle2State;
      }
     
      if (userInputState->state == LONG_PRESS) {
        return helloState;
      }
      break;

    case CLEAN_RINSE :

      if (currentGaggiaState->counter == currentGaggiaState->targetCounter-1) {
        return cleanDoneState;
      }
     
      if (userInputState->state == LONG_PRESS) {
        return helloState;
      }
      break;
  
      case CLEAN_DONE :

      if (userInputState->state == SHORT_PRESS) {
        return helloState;
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
                                NetworkState *networkState,
                                float nowTimeMillis) {
  if (nextGaggiaState->waterThroughGroupHead || nextGaggiaState->waterThroughWand) {
    publishParticleLog("dispense", "Launching Pressure PID");

    waterPumpState->targetPressureInBars = DISPENSING_BAR;

    if (nextGaggiaState->state == PREINFUSION) {
      waterPumpState->targetPressureInBars = PRE_INFUSION_BAR;
    }

    if (nextGaggiaState->state == CLEAN_SOAP ||
        nextGaggiaState->state == CLEAN_RINSE) {
      waterPumpState->targetPressureInBars = BACKFLUSH_BAR;
    }
    
    // The reason we do this here is because PID can't change its target value,
    // so we must create a new one when our target pressure changes..
    PID *thisWaterPumpPID = new PID(&waterPumpState->measuredPressureInBars,  // input
                                    &waterPumpState->pumpDutyCycle,  // output
                                    &waterPumpState->targetPressureInBars,  // target
                                    PID_kP, PID_kI, PID_kD, PID::DIRECT);
    
    // The Gaggia water pump doesn't energize at all below 30 duty cycle.
    // This number range is the 'dutyCycle' of the power we are sending to the water
    // pump.

    double maxOutput = MAX_PUMP_DUTY_CYCLE;
    // For preinfusion, we hold dutyCycle at 30%. When the puck is dry, it provides little
    // backpressure and so the PID will immediately ramp up to max duty cycle, which is NOT
    // want we want for preinfusion.... This is a silly way to use the PID, in general, but works
    // only for preinfusion.    
    if (nextGaggiaState->state == PREINFUSION) {
      maxOutput = MIN_PUMP_DUTY_CYCLE;
    }
    thisWaterPumpPID->SetOutputLimits(MIN_PUMP_DUTY_CYCLE, maxOutput);
    thisWaterPumpPID->SetSampleTime(500);
    thisWaterPumpPID->SetMode(PID::AUTOMATIC);

    delete waterPumpState->waterPumpPID;
    waterPumpState->waterPumpPID = thisWaterPumpPID;
  }

  if (nextGaggiaState->brewHeaterOn) {
    // The reason we do this here is because PID can't change its target value,
    // so we must create a new one when our target temperature changes..
    PID *thisHeaterPID = new PID(&heaterState->measuredTemp, 
                                 &heaterState->heaterShouldBeOn, 
                                 &TARGET_BREW_TEMP, 
                                 PID_kP, PID_kI, PID_kD, PID::DIRECT);
    // The heater is either on or off, there's no need making this more complicated..
    // So the PID either turns the heater on or off.
    thisHeaterPID->SetOutputLimits(0, 1);
    thisHeaterPID->SetMode(PID::AUTOMATIC);
    thisHeaterPID->SetSampleTime(500);

    delete heaterState->heaterPID;
    heaterState->heaterPID = thisHeaterPID;
  }

  if (nextGaggiaState->steamHeaterOn) {
    PID *thisHeaterPID = new PID(&heaterState->measuredTemp, 
                                 &heaterState->heaterShouldBeOn, 
                                 &TARGET_STEAM_TEMP, 
                                 PID_kP, PID_kI, PID_kD, PID::DIRECT);
    // The heater is either on or off, there's no need making this more complicated..
    // So the PID either turns the heater on or off.
    thisHeaterPID->SetOutputLimits(0, 1);
    thisHeaterPID->SetMode(PID::AUTOMATIC);
    thisHeaterPID->SetSampleTime(500);


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
                                NetworkState *networkState,
                                float nowTimeMillis) {

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
    scaleState->tareWeight = scaleState->measuredWeight;
  }

  // Process Record Weight 
  if (currentGaggiaState->recordWeight) {
    scaleState->targetWeight = 
      (scaleState->measuredWeight - scaleState->tareWeight)*BREW_WEIGHT_TO_BEAN_RATIO; 
  }

  if (currentGaggiaState->state == BREWING) {
    // Just finished brewing.. Need to calculate flow rate...

    // The rate is usually expressed as grams/30seconds
    float timeSpent30SecondIntervals = timeSpentInCurrentStateMillis(currentGaggiaState)/(30 * 1000.0);

    float dispendedWaterWeight =  scaleState->targetWeight;

    publishParticleLog("flowRate", "(" + String(dispendedWaterWeight) + "/" + String(timeSpent30SecondIntervals) + ")");

    scaleState->flowRate = dispendedWaterWeight/timeSpent30SecondIntervals;
  }

  // This gives our current state one last chance to log any telemetry.
  sendTelemetryIfNecessary(nowTimeMillis,
                           currentGaggiaState,
                           heaterState,
                           scaleState,
                           waterReservoirState,
                           waterPumpState,
                           networkState);

  // Things we always reset when leaving a state...
  currentGaggiaState->stopTimeMillis = -1;
  currentGaggiaState->counter = -1;
  currentGaggiaState->nextTelemetryMillis = -1;
}

void processCurrentGaggiaState(GaggiaState *currentGaggiaState,  
                               HeaterState *heaterState,
                               ScaleState *scaleState,
                               DisplayState *displayState,
                               WaterReservoirState *waterReservoirState,
                               WaterPumpState *waterPumpState,
                               NetworkState *networkState,
                               float nowTimeMillis) {

  // 
  // Process Display
  //
  displayState->display1 = updateDisplayLine(currentGaggiaState->display1, 
                                             1, 
                                             currentGaggiaState,
                                             heaterState, 
                                             scaleState, 
                                             waterPumpState,
                                             networkState,
                                             displayState->display1);

  displayState->display2 = updateDisplayLine(currentGaggiaState->display2, 
                                             2, 
                                             currentGaggiaState,
                                             heaterState, 
                                             scaleState, 
                                             waterPumpState,
                                             networkState,
                                             displayState->display2);

  displayState->display3 = updateDisplayLine(currentGaggiaState->display3, 
                                             3, 
                                             currentGaggiaState,
                                             heaterState, 
                                             scaleState,  
                                             waterPumpState,
                                             networkState,
                                             displayState->display3);

  displayState->display4 = updateDisplayLine(currentGaggiaState->display4, 
                                             4, 
                                             currentGaggiaState,
                                             heaterState, 
                                             scaleState, 
                                             waterPumpState,
                                             networkState,
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

  if (currentGaggiaState->fillingReservoir) {

    if (doesWaterReservoirNeedFilling()) {
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

  if (currentGaggiaState->waterThroughGroupHead || currentGaggiaState->waterThroughWand) {
    if (currentGaggiaState->state == CLEAN_SOAP || currentGaggiaState->state == CLEAN_RINSE) {
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
    networkState->connected = false;
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
      networkState->connected = true;
    }
  }
}


void readScaleState(NAU7802 myScale, ScaleState *scaleState) {
  scaleState->measuredWeight = 0;

  if (myScale.available() == true) {
    float scaleReading = myScale.getReading();
  
    // Spreadsheet which shows calibration for this scale:
    // https://docs.google.com/spreadsheets/d/1z3atpMJ9mnRWZPx-S3QPc0Lp2lXNPtheInoUFgV0nMo/edit?usp=sharing 
    // weightInGrams = scaleReading * SCALE_FACTOR + SCALE_OFFSET
    // y = 6.33E-04*x + -13.2
  
    float weightInGrams = scaleReading * SCALE_FACTOR + SCALE_OFFSET;

    // This is a rotating buffer
    scaleState->avgWeights[scaleState->avgWeightIndex++] = weightInGrams;
    if(scaleState->avgWeightIndex == SCALE_SAMPLE_SIZE) scaleState->avgWeightIndex = 0;

    float avgWeight = 0;
    for (int index = 0 ; index < SCALE_SAMPLE_SIZE ; index++)
      avgWeight += scaleState->avgWeights[index];
    avgWeight /= SCALE_SAMPLE_SIZE;

    scaleState->measuredWeight = avgWeight;
  }
}

boolean isButtonPressedRightNow() {
  return userButton.isPressed();
}

float timeSpentInCurrentStateMillis(GaggiaState *currentGaggiaState) {
  return millis() - currentGaggiaState->stateEnterTimeMillis;
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

// The solenoid valve allows water to through to grouphead.
void startDispensingWater(boolean turnOnSolenoidValve) {
  publishParticleLog("dispenser", "dispensingOn");

  readPumpState(&waterPumpState);  

  // This triggers the PID to use previous and current
  // measured values to calculate the current required
  // dutyCycle of the water pump
  // By default, it will update output values every 200ms, regardless
  // of how often we call Compute()
  waterPumpState.waterPumpPID->Compute();
  publishParticleLog("dispenser", "pumpDutyCycle: " + String(waterPumpState.pumpDutyCycle));

  // The zero crossings from the incoming AC sinewave will trigger
  // this interrupt handler, which will modulate the power duty cycle to
  // the water pump..
  //
  // This will not trigger unless water dispenser is running.
  //
  attachInterrupt(ZERO_CROSS_DISPENSE_POT, handleZeroCrossingInterrupt, RISING, 0);

  if (turnOnSolenoidValve) {
    digitalWrite(SOLENOID_VALVE_SSR, HIGH);
  } else {
    digitalWrite(SOLENOID_VALVE_SSR, LOW);
  }
}

void readPumpState(WaterPumpState *waterPumpState) {
  // reading from first channel of the 1015

  // no pressure ~1100
  int rawPressure = ads1115.readADC_SingleEnded(0);
  int normalizedPressureInBars = (rawPressure-PRESSURE_SENSOR_OFFSET)/PRESSURE_SENSOR_SCALE_FACTOR;

  publishParticleLog("pump", "dutyCycle: " + String(waterPumpState->pumpDutyCycle) + "', normalizedPressure: " + String(normalizedPressureInBars));

  waterPumpState->measuredPressureInBars = normalizedPressureInBars;
}

void stopDispensingWater() {
  publishParticleLog("dispenser", "dispensingOff");

  detachInterrupt(ZERO_CROSS_DISPENSE_POT);

  digitalWrite(SOLENOID_VALVE_SSR, LOW);
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
                        GaggiaState *gaggiaState,
                        HeaterState *heaterState,
                        ScaleState *scaleState,
                        WaterPumpState *waterPumpState,
                        NetworkState *networkState,
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
                                                "{adjustedWeight}",
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
              if (lineToDisplay == "") {
                // One count consists of both the clean and rest... so we only have
                // targetCounter/2 total clean passes.
                lineToDisplay = decodeMessageIfNecessary(message,
                                                       "{currentPass}/{targetPass}",
                                                       (gaggiaState->counter/2)+1,
                                                       (gaggiaState->targetCounter)/2,
                                                       " pass");
                if (lineToDisplay == "") {
                  // One count consists of both the clean and rest... so we only have
                  // targetCounter/2 total clean passes.
                  lineToDisplay = decodeMessageIfNecessary(message,
                                                       "{pumpDutyCycle}/{maxDutyCycle}",
                                                       waterPumpState->pumpDutyCycle,
                                                       MAX_PUMP_DUTY_CYCLE,
                                                       " %");
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

long filterLowWeight(long weight) {
  if (abs(weight) < LOW_WEIGHT_THRESHOLD) {
    return 0;
  } else {
    return weight;
  }
}

boolean shouldTurnOnHeater(float nowTimeMillis,
                           HeaterState *heaterState) {
                        
  // By default, it will update output values every 200ms, regardless
  // of how often we call Compute()
  heaterState->heaterPID->Compute();

  if (heaterState->heaterShouldBeOn > 0 ) {
    return true;
  } else {
    return false;
  }
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

int _setPreInfusionState(String _) {
  
  // by jumping to this state, we are bypassing earlier states.. so we need to
  // fake any data that should have been collected.
  scaleState.tareWeight = 320;
  scaleState.targetWeight = 100;

  manualNextGaggiaState = preinfusionState;
  return 1;
}

int _setHeatingState(String _) {
  
  // by jumping to this state, we are bypassing earlier states.. so we need to
  // fake any data that should have been collected.
  scaleState.tareWeight = 320;
  scaleState.targetWeight = 100;

  manualNextGaggiaState = heatingToBrewState;
  return 1;
}

int _setCoolingState(String _) {
  manualNextGaggiaState = coolingState;
  return 1;
}

int _setSteamingState(String _) {
  manualNextGaggiaState = steamingState;
  return 1;
}

int _setHelloState(String _) {
  manualNextGaggiaState = helloState;
  return 1;
}

int _setDispenseHotWater(String _) {
  manualNextGaggiaState = dispenseHotWaterState;
  return 1;
}


int _setPID_kP(String _PID_kP) {
  
  PID_kP = _PID_kP.toFloat();

  return 1;
}

int _setPID_kI(String _PID_kI) {
  
  PID_kI = _PID_kI.toFloat();

  return 1;
}

int _setPID_kD(String _PID_kD) {
  
  PID_kD = _PID_kD.toFloat();

  return 1;
}

int _setDispensingBar(String _dispensingBar) {
  
  DISPENSING_BAR = _dispensingBar.toFloat();

  return 1;
}

int _setPreInfusionBar(String _preInfusionBar) {
  
  DISPENSING_BAR = _preInfusionBar.toFloat();

  return 1;
}

boolean doesWaterReservoirNeedFilling() {
  
  int needFilling = digitalRead(WATER_RESERVOIR_SENSOR);

  if (needFilling == HIGH) {
    return true;
  } else {
    return false;
  }
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

void sendTelemetryIfNecessary(float nowTimeMillis,
                              GaggiaState *gaggiaState,
                              HeaterState *heaterState,
                              ScaleState *scaleState,
                              WaterReservoirState *waterReservoirState,
                              WaterPumpState *waterPumpState,
                              NetworkState *networkState) {

  if (TELEMETRY_ENABLED && networkState->connected) {
        // we want telemetry to be available for all non-rest states...
        // recall the system returns to hello after 15 minutes of inactivity.
        MQTTConnect();
  }

  if (gaggiaState->state == BREWING || gaggiaState->state == PREINFUSION) {
    if (nowTimeMillis > gaggiaState->nextTelemetryMillis) {

      sendMessageToCloud(
        String("v1.1, ") + // telemetery version
        String(getStateName(gaggiaState->state)) + String(", ") + // state name
        String(scaleState->measuredWeight - scaleState->tareWeight) + String(", ") + // extracted liquid weight in grams
        String(scaleState->targetWeight) + String(", ") + // target liquid weight in grams 
        String(waterPumpState->measuredPressureInBars) + String(", ") +  // measured pressure in bars 
        String(waterPumpState->targetPressureInBars) + String(", ") + // target pressure in bars 
        String(waterPumpState->pumpDutyCycle) + String(", ")  // pump duty cycle 
      , &mqttRoboGaggiaTelemetryTopic, networkState);


      gaggiaState->nextTelemetryMillis = nowTimeMillis + TELEMETRY_PERIOD_MILLIS;
    }
  }
}

void sendMessageToCloud(const char* message, Adafruit_MQTT_Publish* topic, NetworkState *networkState) {
  if (TELEMETRY_ENABLED && networkState->connected) {
    if (topic->publish(message)) {
        publishParticleLog("mqtt", "Message SUCCESS (" + String(message) + ").");
    } else {
        publishParticleLog("mqtt", "Message FAIL (" + String(message) + ").");
    }
  }
}


void MQTTConnect() {
    int8_t ret;

    // Stop if already connected.
    if (mqtt.connected()) {
        return;
    }

    // Note that reconnecting more than 20 times per minute will cause a temporary ban
    // on account
    int result = mqtt.connect();
    if (result != 0) {
      publishParticleLog("mqtt", "MQTT connect error: " + String(mqtt.connectErrorString(ret)));
    }

    // give up for now...
    // while ((ret = mqtt.connect()) != 0) { // connect will return 0 for connected
    //     publishParticleLog("mqtt", "Retrying MQTT connect from error: " + String(mqtt.connectErrorString(ret)));
    //     mqtt.disconnect();
    //     delay(250); 
    // }
}

void MQTTDisconnect() {
    mqtt.disconnect();
}