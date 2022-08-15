/*
 * Project roboGaggia
 * Description:
 * Author:
 * Date:
 */

#include <Wire.h> 
#include <SPI.h>
#include "SerLCD.h"
#include "SparkFun_Qwiic_Scale_NAU7802_Arduino_Library.h"

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

// For turning on the heater
#define HEATER_brew   D7
#define HEATER_steam  D8

// User Input
// Pull high to trigger
#define BUTTON  D3


//
// Constants
//

// These are constants of proportionality for the three PID 
// components: current (p), past (i), and future (d)
int kp = 9.1;   
int ki = 0.3;   
int kd = 1.8;

float TARGET_BREW_TEMP = 45; // should be 103
float TOO_HOT_TEMP = 55; // should be 130
float TARGET_STEAM_TEMP = 65; // should be 140

// How long the button has to be pressed before it is considered
// a long press
float BUTTON_LONG_PRESS_DURATION_MILLIS = 1000;

float BREW_MEASURED_WEIGHT_MULTIPLIER = 2.0;


//
// State
//
struct ScaleState {

  // current weight
  long measuredWeight = 0;

  // recorded weight
  long recordedWeight = 0; 

  // recorded weight of cup meant be used when
  // measuring the weight of beans or brew  
  long tareWeight = 0;
} scaleState;


struct HeaterState {

  // current temp
  double measuredTemp;

  // The difference between measured temperature and set temperature
  float previousTempError = 0;

  // When last measurement was taken
  // Used for heater duration calculation
  float previousSampleTime = millis();

  // Used to track ongoing heat cycles...
  float heaterStarTime = -1;
  float heaterDurationMillis = -1;


} brewHeaterState, steamHeaterState;


//
// the state of the button
//

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
} userInputState;


// This describes the overall state of the robo Gaggia
enum GaggiaStateEnum {
  HELLO = 0, 
  TARE_CUP_1 = 1, 
  MEASURE_BEANS = 2, 
  TARE_CUP_2 = 3, 
  HEATING_TO_BREW = 4, 
  BREWING = 5, 
  DONE_BREWING = 6, 
  HEATING_TO_STEAM = 7, 
  STEAMING = 8, 
  COOL_START = 9, 
  COOLING = 10, 
  COOL_DONE = 11, 
};
struct GaggiaState {
   enum GaggiaStateEnum state;   
   char *display1 = "";
   char *display2 = "";
   char *display3 = "";
   char *display4 = "";
   boolean  brewHeaterOn = false;
   boolean  steamHeaterOn = false;
   boolean  tareScale = false;
   boolean  dispenseWater = false;
   boolean  recordWeight = false;
} roboGaggiaState;
struct GaggiaState helloState;
struct GaggiaState tareCup1State;
struct GaggiaState measureBeansState;
struct GaggiaState tareCup2State;
struct GaggiaState heatingToBrewState;
struct GaggiaState brewingState;
struct GaggiaState doneBrewingState;
struct GaggiaState heatingToSteamState;
struct GaggiaState steamingState;
struct GaggiaState coolStartState;
struct GaggiaState coolingState;
struct GaggiaState coolDoneState;

struct GaggiaState currentGaggiaState = helloState;


SerLCD lcd; // Initialize the library with default I2C address 0x72

NAU7802 myScale; //Create instance of the NAU7802 class

// setup() runs once, when the device is first turned on.
void setup() {
  
  // I2C Setup
  Wire.begin();

  // LCD Setup
  lcd.begin(Wire); //Set up the LCD for I2C communication

  lcd.setBacklight(255, 255, 255); //Set backlight to bright white
  lcd.setContrast(5); //Set contrast. Lower to 0 for higher contrast.

  lcd.clear(); //Clear the display - this moves the cursor to home position as well
  
  // Scale check
  if (myScale.begin() == false)
  {
    Log.error("Scale not detected!");
  }
  
  // setup MAX6675 to read the temperature from thermocouple
  pinMode(MAX6675_CS_brew, OUTPUT);
  pinMode(MAX6675_CS_steam, OUTPUT);
  pinMode(MAX6675_SO_brew, INPUT);
  pinMode(MAX6675_SO_steam, INPUT);
  pinMode(MAX6675_SCK, OUTPUT);
  
  // external heater elements
  pinMode(HEATER_brew, OUTPUT);
  pinMode(HEATER_steam, OUTPUT);

  // user input .. will be used mostly for one-click,
  // but also will support 'hold/long-press' to give it
  // two modes capability
  pinMode(BUTTON, INPUT_PULLDOWN);

  Particle.variable("measuredBrewTemp", brewHeaterState.measuredTemp);
  Particle.variable("measuredSteamTemp", steamHeaterState.measuredTemp);

  // Define all possible states of RoboGaggia
  helloState.state = HELLO; 
  helloState.display1 =           "Hello. I am RoboGaggia!";
  helloState.display4 =           "<-- Click to Steam     Click to Brew -->";
  helloState.brewHeaterOn = true; 

  tareCup1State.state = TARE_CUP_1; 
  tareCup1State.display1 =         "Place bean cup on tray";
  tareCup1State.display4 =         "                    Click when Ready -->";
  tareCup1State.brewHeaterOn = true; 
  tareCup1State.tareScale = true; 

  measureBeansState.state = MEASURE_BEANS; 
  measureBeansState.display1 =    "Add beans to cup";
  measureBeansState.display2 =    "${measuredWeight}/${targetBeanWeight}";
  measureBeansState.display4 =    "                     Click when Ready -->";
  measureBeansState.brewHeaterOn = true; 
  measureBeansState.recordWeight = true; 

  tareCup2State.state = TARE_CUP_2; 
  tareCup2State.display1 =        "Grind beans, load portafiler,";
  tareCup2State.display2 =        "place empty cup on tray";
  tareCup2State.display4 =        "                   Click when Ready -->";
  tareCup2State.brewHeaterOn = true; 
  tareCup2State.tareScale = true; 

  heatingToBrewState.state = HEATING_TO_BREW; 
  heatingToBrewState.display1 =   "Heating to brew. Leave cup on tray.";
  heatingToBrewState.display2 =   "${measuredBrewTemp}/${targetBrewTemp}";
  heatingToBrewState.display4 =   "Please wait ...";
  heatingToBrewState.brewHeaterOn = true; 

  brewingState.state = BREWING; 
  brewingState.display1 =         "Brewing.";
  brewingState.display2 =         "${measuredWeight}/${targetBrewWeight}";
  brewingState.display4 =         "Please wait ...";
  brewingState.brewHeaterOn = true; 
  brewingState.dispenseWater = true; 

  doneBrewingState.state = DONE_BREWING; 
  doneBrewingState.display1 =    "Done brewing.";
  doneBrewingState.display2 =    "Remove cup from tray.";
  doneBrewingState.display4 =    "                     Click when Ready -->";
  doneBrewingState.brewHeaterOn = true; 

  heatingToSteamState.state = HEATING_TO_STEAM; 
  heatingToSteamState.display1 =   "Heating to steam.";
  heatingToSteamState.display2 =   "${measuredSteamTemp}/${targetSteamTemp}";
  heatingToSteamState.display4 =   "Please wait ...";
  heatingToSteamState.brewHeaterOn = true; 
  heatingToSteamState.steamHeaterOn = true; 

  steamingState.state = STEAMING; 
  steamingState.display1 =         "Operate steam wand.";
  steamingState.display4 =    "                        Click when Done -->";
  steamingState.brewHeaterOn = true; 
  steamingState.steamHeaterOn = true; 

  coolStartState.state = COOL_START; 
  coolStartState.display1 =    "Place empty cup on tray.";
  coolStartState.display4 =    "                     Click when Ready -->";

  coolingState.state = COOLING; 
  coolingState.display1 =   "Dispensing water to cool";
  coolingState.display2 =   "${measuredBrewTemp}/${targetBrewTemp}";
  coolingState.display4 =   "Please wait ...";
  coolingState.dispenseWater = true; 

  coolDoneState.state = COOL_DONE;
  coolDoneState.display1 =    "Remove cup and discard water.";
  coolDoneState.display4 =    "                     Click when Ready -->";
  coolDoneState.brewHeaterOn = true; 

  // Wait for a USB serial connection for up to 15 seconds
  waitFor(Serial.isConnected, 15000);
}

void readScaleState(NAU7802 myScale, ScaleState *scaleState) {
  scaleState->measuredWeight = 0;

  if (myScale.available() == true) {
    scaleState->measuredWeight = myScale.getReading();
    Log.error("Scale Reading: " + String(scaleState->measuredWeight));
  }
}

void readUserInputState(boolean isButtonPressedRightNow, float nowTimeMillis, UserInputState *userInputState) {
  // Derive current user input state
  userInputState->state = IDLE;
  if (isButtonPressedRightNow) {
    if (userInputState->buttonPressStartTimeMillis < 0) {  
      // rising edge detected...
      userInputState->buttonPressStartTimeMillis = nowTimeMillis;
    }
  } else {
    if (userInputState->buttonPressStartTimeMillis > 0) {  
      // falling edge detected...
      if (nowTimeMillis - userInputState->buttonPressStartTimeMillis < BUTTON_LONG_PRESS_DURATION_MILLIS) {
        userInputState->state = SHORT_PRESS;
      } else {
        userInputState->state = LONG_PRESS;
      }
  
      userInputState->buttonPressStartTimeMillis = -1;
    }
  }
}

GaggiaState getCurrentGaggiaState(GaggiaState currentGaggiaState, 
                                  HeaterState brewHeaterState,
                                  HeaterState steamHeaterState,
                                  ScaleState scaleState,
                                  UserInputState userInputState) {

  switch (currentGaggiaState.state) {

    case HELLO :

      if (userInputState.state == SHORT_PRESS) {
        if (brewHeaterState.measuredTemp >= TOO_HOT_TEMP) {
          return coolStartState;
        } else {
          return tareCup1State;
        }
      }

      if (userInputState.state == LONG_PRESS) {
        return heatingToSteamState;
      }
      break;
      
    case TARE_CUP_1 :

      if (userInputState.state == SHORT_PRESS) {
        return measureBeansState;
      }
      break;

    case MEASURE_BEANS :

      if (userInputState.state == SHORT_PRESS) {
        return tareCup2State;
      }
      break;  

    case TARE_CUP_2 :

      if (userInputState.state == SHORT_PRESS) {
        return heatingToBrewState;
      }
      break;

    case HEATING_TO_BREW :

      if (brewHeaterState.measuredTemp >= TARGET_BREW_TEMP) {
        return brewingState;
      }
      break;

    case BREWING :

      if (scaleState.measuredWeight >= scaleState.recordedWeight*BREW_MEASURED_WEIGHT_MULTIPLIER) {
        return doneBrewingState;
      }
      break;

    case DONE_BREWING :

      if (userInputState.state == SHORT_PRESS) {
        return helloState;
      }
      break;

    case HEATING_TO_STEAM :

      if (steamHeaterState.measuredTemp >= TARGET_STEAM_TEMP) {
        return steamingState;
      }
      break;

    case STEAMING :

      if (userInputState.state == SHORT_PRESS) {
        return helloState;
      }
      break;
      
    case COOL_START :

      if (userInputState.state == SHORT_PRESS) {
        return coolingState;
      }
      break;

    case COOLING :

      if (brewHeaterState.measuredTemp <= TARGET_BREW_TEMP) {
        return coolDoneState;
      }
      break;

    case COOL_DONE :

      if (userInputState.state == SHORT_PRESS) {
        return helloState;
      }
      break;  
  } 

  return currentGaggiaState;
}  

boolean shouldTurnOnHeater(HeaterState *heaterState,
                           float targetTemp,
                           float nowTimeMillis) {
                        
  boolean shouldTurnOnHeater = false;

  if (heaterState->heaterStarTime > 0 ) {
    // we're in a heat cycle....

    if ((nowTimeMillis - heaterState->heaterStarTime) > heaterState->heaterDurationMillis) {
      // done this heat cycle!
      Log.error("Stopping current heat cycle.");

      heaterState->heaterStarTime = -1;
      shouldTurnOnHeater = false;
    }
  } else {
    // determine if we should be in a heat cycle ...

    heaterState->heaterDurationMillis = 
      calculateHeaterPulseDurationMillis(heaterState->measuredTemp, 
                                         targetTemp, 
                                         &(heaterState->previousTempError), 
                                         &(heaterState->previousSampleTime));
    if (heaterState->heaterDurationMillis > 0) {
      Log.error("Starting heat cycle for '" + String(heaterState->heaterDurationMillis) + "' milliseconds.");
      heaterState->heaterStarTime = nowTimeMillis;

      shouldTurnOnHeater = true;
    }
  }

  return shouldTurnOnHeater;
}

void startDispensingWater() {
  // NJD TODO
}

void stopDispensingWater() {
  // NJD TODO
}

void processCurrentGaggiaState(GaggiaState currentGaggiaState, 
                               HeaterState *brewHeaterState,
                               HeaterState *steamHeaterState,
                               ScaleState *scaleState,
                               float nowTimeMillis) {

  // 
  // Process Display
  //
  lcd.clear();
  updateDisplayLine(currentGaggiaState.display1, 1);
  updateDisplayLine(currentGaggiaState.display2, 2);
  updateDisplayLine(currentGaggiaState.display3, 3);
  updateDisplayLine(currentGaggiaState.display4, 4);

  // Process Heaters
  //
  // Even though a heater may be 'on' during this state, the control system
  // for the heater turns it off and on intermittently in an attempt to regulate
  // the temperature around the target temp.
  if (currentGaggiaState.brewHeaterOn) {
    if (shouldTurnOnHeater(brewHeaterState, TARGET_BREW_TEMP, nowTimeMillis)) {
      Log.error("Turning on brew heater.");
      turnBrewHeaterOn();
    } else {
      Log.error("Turning off brew heater.");
      turnBrewHeaterOff();
    } 
  } else {
    turnBrewHeaterOff();
  }

  if (currentGaggiaState.steamHeaterOn) {
    if (shouldTurnOnHeater(steamHeaterState, TARGET_STEAM_TEMP, nowTimeMillis)) {
      Log.error("Turning on steam heater.");
      turnSteamHeaterOn();
    } else {
      Log.error("Turning off steam heater.");
      turnSteamHeaterOff();
    } 
  } else {
    turnSteamHeaterOff();
  }

  // Process Tare Scale
  if (currentGaggiaState.tareScale) {
    scaleState->tareWeight = scaleState->measuredWeight;
  }

  // Process Dispense Water 
  if (currentGaggiaState.dispenseWater) {
    Log.error("Start dispending water.");
    startDispensingWater();
  } else {
    Log.error("Stop dispending water.");
    stopDispensingWater();
  }

  // Process Record Weight 
  if (currentGaggiaState.recordWeight) {
    scaleState->measuredWeight = scaleState->measuredWeight;
  }
}


void readHeaterTemperature(int CHIP_SELECT_PIN, int SERIAL_OUT_PIN, int SERIAL_CLOCK_PIN, HeaterState *heaterState) {

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
  } else {

    // The lower three bits (0,1,2) are discarded status bits
    measuredValue >>= 3;

    // The remaining bits are the number of 0.25 degree (C) counts
    heaterState->measuredTemp = measuredValue*0.25;
  }
}

void loop() {
  
  float nowTimeMillis = millis();  

  // First we read all inputs... even if these inputs aren't always needed for
  // our given state.. it's easier to just do it here as it costs very little.

  readScaleState(myScale, &scaleState);
  
  readUserInputState(isButtonPressedRightNow(), nowTimeMillis, &userInputState);

  readHeaterTemperature(MAX6675_CS_brew, MAX6675_SO_brew, MAX6675_SCK, &brewHeaterState);  
  Log.error("measuredBrewTempC '" + String(brewHeaterState.measuredTemp) + "'.");

  readHeaterTemperature(MAX6675_CS_steam, MAX6675_SO_steam, MAX6675_SCK, &steamHeaterState);  
  Log.error("measuredSteamTempC '" + String(  steamHeaterState.measuredTemp) + "'.");

  // Given current input, determine current Gaggia state...
  currentGaggiaState = getCurrentGaggiaState(currentGaggiaState, 
                                             brewHeaterState, 
                                             steamHeaterState, 
                                             scaleState, 
                                             userInputState);

  // Perform actions given current Gaggia state...
  // This step does also mutate current state
  // (e.g. record weight of beans, tare measuring cup)
  processCurrentGaggiaState(currentGaggiaState, 
                            &brewHeaterState, 
                            &steamHeaterState, 
                            &scaleState,
                            nowTimeMillis);

  delay(5000);
}

void turnBrewHeaterOn() {
  digitalWrite(HEATER_brew, HIGH);
}

void turnBrewHeaterOff() {
  digitalWrite(HEATER_brew, LOW);
}

void turnSteamHeaterOn() {
  digitalWrite(HEATER_steam, HIGH);
}

void turnSteamHeaterOff() {
  digitalWrite(HEATER_steam, LOW);
}

boolean isButtonPressedRightNow() {
  return digitalRead(BUTTON) == HIGH;
}

// line starts at 1
void updateDisplayLine(const char* message, int line) {
  lcd.setCursor(0,line-1);
  lcd.print(message);
}

int calculateHeaterPulseDurationMillis(double currentTempC, float targetTempC, float *previousTempError, float *previousSampleTime) {

  float currentTempError = targetTempC - currentTempC;

  // Calculate the P value
  int PID_p = kp * currentTempError;

  // Calculate the I value in a range on +-3
  int PID_i = 0;
  float threshold = 3.0;
  if(-1*threshold < currentTempError < threshold)
  {
    PID_i = PID_i + (ki * currentTempError);
  }

  //For derivative we need real time to calculate speed change rate
  float currentSampleTime = millis();                            // actual time read
  float elapsedTime = (currentSampleTime - *previousSampleTime) / 1000; 

  // Now we can calculate the D value - the derivative or slope.. which is a means
  // of predicting future value
  int PID_d = kd*((currentTempError - *previousTempError)/elapsedTime);

  *previousTempError = currentTempError;     //Remember to store the previous error for next loop.
  *previousSampleTime = currentSampleTime; // for next cycle.. we need to remember previousTime
  
  //vFinal total PID value is the sum of P + I + D
  int currentOutput = PID_p + PID_i + PID_d;

  //We define PWM range between 0 and 255
  if(currentOutput < 0)
  {    currentOutput = 0;    }
  if(currentOutput > 255)  
  {    currentOutput = 255;  }

  return currentOutput;
}