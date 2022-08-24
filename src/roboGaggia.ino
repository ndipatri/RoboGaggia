/*
 * Project roboGaggia
 * Description:
 * Author:
 * Date:
 */

#include <Wire.h> 
#include <string.h> 
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

// Output - For turning on the heater
#define HEATER_brew   D7
#define HEATER_steam  D8

// User Input
// Pull high to trigger
#define BUTTON  RX 

// Output - For dispensing water
#define WATER_DISPENSER  TX


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
float TARGET_STEAM_TEMP = 100; // should be 140
float TARGET_BEAN_WEIGHT = 22; // grams

// How long the button has to be pressed before it is considered
// a long press
float BUTTON_LONG_PRESS_DURATION_MILLIS = 1000;

// This is the ration of final espresso weight compared to
// that of the ground beans.  Typically this is 2-to-1
float BREW_MEASURED_WEIGHT_MULTIPLIER = 2.0;

int SCALE_OFFSET = 573;
double SCALE_FACTOR = -.01;

// Below which we consider weight to be 0
int LOW_WEIGHT_THRESHOLD = 4;

//
// State
//

struct ScaleState {

  long measuredWeight = 0;

  // this will be the measuredWegith - tareWeight * BREW_MEASURED_WEIGHT_MULTIPLIER
  // at the moment this value is recorded...
  long targetWeight = 0; 

  // recorded weight of cup meant be used when
  // measuring the weight of beans or brew  
  long tareWeight = 0;

} scaleState;
void readScaleState(NAU7802 myScale, ScaleState *scaleState);
NAU7802 myScale; //Create instance of the NAU7802 class


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
void readHeaterState(int CHIP_SELECT_PIN, int SERIAL_OUT_PIN, int SERIAL_CLOCK_PIN, HeaterState *heaterState);
boolean shouldTurnOnHeater(HeaterState *heaterState, float targetTemp, float nowTimeMillis);



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
void readUserInputState(boolean isButtonPressedRightNow, float nowTimeMillis, UserInputState *userInputState);

struct DisplayState {
  String display1;
  String display2;
  String display3;
  String display4;

} displayState;

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
} 
helloState,
tareCup1State,
measureBeansState,
tareCup2State,
heatingToBrewState,
brewingState,
doneBrewingState,
heatingToSteamState,
steamingState,
coolStartState,
coolingState,
coolDoneState,

currentGaggiaState;

SerLCD display; // Initialize the library with default I2C address 0x72

// Using all current state, we derive the next state of the system
GaggiaState getNextGaggiaState(GaggiaState currentGaggiaState, 
                               HeaterState brewHeaterState,
                               HeaterState steamHeaterState,
                               ScaleState scaleState,
                               UserInputState userInputState);

// Once we know the state, we affect appropriate change in our
// system attributes (e.g. temp, display)
// Things we do while we are in a state 
void processCurrentGaggiaState(GaggiaState currentGaggiaState,
                               HeaterState *brewHeaterState,
                               HeaterState *steamHeaterState,
                               ScaleState *scaleState,
                               DisplayState *displayState,
                               float nowTimeMillis);
// Things we do when we leave a state
void processOutgoingGaggiaState(GaggiaState currentGaggiaState,
                                GaggiaState nextGaggiaState, 
                                HeaterState *brewHeaterState,
                                HeaterState *steamHeaterState,
                                ScaleState *scaleState,
                                DisplayState *displayState,
                                float nowTimeMillis);
GaggiaState returnToHelloState(ScaleState *scaleState);

boolean firstLoop = true;

// setup() runs once, when the device is first turned on.
void setup() {
  
  // I2C Setup
  Wire.begin();

  // LCD Setup
  display.begin(Wire); //Set up the LCD for I2C communication

  display.setBacklight(255, 255, 255); //Set backlight to bright white
  display.setContrast(5); //Set contrast. Lower to 0 for higher contrast.

  display.clear(); //Clear the display - this moves the cursor to home position as well
  
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

  // water dispenser
  pinMode(WATER_DISPENSER, OUTPUT);


  // // Useful state exposed to Particle web console
  Particle.variable("brewTempC", brewHeaterState.measuredTemp);
  Particle.variable("steamTempC", steamHeaterState.measuredTemp);

  Particle.variable("measuredWeightG", scaleState.measuredWeight);
  Particle.variable("tareWeightC", scaleState.tareWeight);
  Particle.variable("SCALE_OFFSET", SCALE_OFFSET);
  Particle.variable("SCALE_FACTOR", SCALE_FACTOR);
  Particle.function("setScaleOffset", setScaleOffset);
  Particle.function("setScaleFactor", setScaleFactor);

  Particle.variable("currentGaggiaState", currentGaggiaState.state);
 


  // Define all possible states of RoboGaggia
  helloState.state = HELLO; 
  helloState.display1 =            "Hello.              ";
  helloState.display2 =            "Clear scale surface.";
  helloState.display3 =            "Click to Brew,      ";
  helloState.display4 =            "Hold for Steam      ";
  helloState.brewHeaterOn = true; 
  // we tare here so the weight of the scale itself isn't shown
  // when we are measuring things...
  tareCup1State.tareScale = true; 


  tareCup1State.state = TARE_CUP_1; 
  tareCup1State.display1 =         "Place cup on tray.  ";
  tareCup1State.display2 =         "{measuredWeight}";
  tareCup1State.display3 =         "                    ";
  tareCup1State.display4 =         "Click when Ready    ";
  tareCup1State.brewHeaterOn = true; 
  tareCup1State.tareScale = true; 

  measureBeansState.state = MEASURE_BEANS; 
  measureBeansState.display1 =     "Add beans to cup.   ";
  measureBeansState.display2 =     "{adjustedWeight}/{targetBeanWeight}";
  measureBeansState.display3 =     "                    ";
  measureBeansState.display4 =     "Click when Ready    ";
  measureBeansState.brewHeaterOn = true; 
  measureBeansState.recordWeight = true; 

  tareCup2State.state = TARE_CUP_2; 
  tareCup2State.display1 =         "Grind & load beans, ";
  tareCup2State.display2 =         "Put cup on tray.    ";
  tareCup2State.display3 =         "                    ";
  tareCup2State.display4 =         "Click when Ready    ";
  tareCup2State.brewHeaterOn = true; 
  tareCup2State.tareScale = true; 

  heatingToBrewState.state = HEATING_TO_BREW; 
  heatingToBrewState.display1 =    "Heating to brew.    ";
  heatingToBrewState.display2 =    "Leave cup on tray.  ";
  heatingToBrewState.display3 =    "{measuredBrewTemp}/{targetBrewTemp}";
  heatingToBrewState.display4 =    "Please wait ...     ";
  heatingToBrewState.brewHeaterOn = true; 

  brewingState.state = BREWING; 
  brewingState.display1 =          "Brewing.            ";
  brewingState.display2 =          "{adjustedWeight}/{targetBrewWeight}";
  brewingState.display3 =          "                    ";
  brewingState.display4 =          "Please wait ...     ";
  brewingState.brewHeaterOn = true; 
  brewingState.dispenseWater = true; 

  doneBrewingState.state = DONE_BREWING; 
  doneBrewingState.display1 =      "Done brewing.       ";
  doneBrewingState.display2 =      "Remove cup.         ";
  doneBrewingState.display3 =      "                    ";
  doneBrewingState.display4 =      "Click when Ready    ";
  doneBrewingState.brewHeaterOn = true; 

  heatingToSteamState.state = HEATING_TO_STEAM; 
  heatingToSteamState.display1 =   "Heating to steam.   ";
  heatingToSteamState.display2 =   "{measuredSteamTemp}/{targetSteamTemp}";
  heatingToSteamState.display3 =   "                    ";
  heatingToSteamState.display4 =   "Please wait ...     ";
  heatingToSteamState.brewHeaterOn = true; 
  heatingToSteamState.steamHeaterOn = true; 

  steamingState.state = STEAMING; 
  steamingState.display1 =         "Operate steam wand. ";
  steamingState.display2 =         "                    ";
  steamingState.display3 =         "                    ";
  steamingState.display4 =         "Click when Done     ";
  steamingState.brewHeaterOn = true; 
  steamingState.steamHeaterOn = true; 

  coolStartState.state = COOL_START; 
  coolStartState.display1 =        "Place cup on tray.  ";
  coolStartState.display2 =        "                    ";;
  coolStartState.display3 =        "                    ";;
  coolStartState.display4 =        "Click when Ready    ";

  coolingState.state = COOLING; 
  coolingState.display1 =          "Dispensing to cool  ";
  coolingState.display2 =          "{measuredBrewTemp}/{targetBrewTemp}";
  coolingState.display3 =          "                    ";
  coolingState.display4 =          "Please wait ...     ";
  coolingState.dispenseWater = true; 

  coolDoneState.state = COOL_DONE;
  coolDoneState.display1 =         "Discard water.      ";
  coolDoneState.display2 =         "                    ";
  coolDoneState.display3 =         "                    ";
  coolDoneState.display4 =         "Click when Ready    ";
  coolDoneState.brewHeaterOn = true; 

  currentGaggiaState = helloState;

  // Wait for a USB serial connection for up to 15 seconds
  waitFor(Serial.isConnected, 15000);
}

int setScaleOffset(String offsetString) {
  SCALE_OFFSET = atoi(offsetString);
  
  return 0;
}

int setScaleFactor(String factorString) {
  SCALE_FACTOR = atoi(factorString);
  
  return 0;
}

void loop() {
  
  float nowTimeMillis = millis();  

  // First we read all inputs... even if these inputs aren't always needed for
  // our given state.. it's easier to just do it here as it costs very little.

  readScaleState(myScale, &scaleState);
  
  readUserInputState(isButtonPressedRightNow(), nowTimeMillis, &userInputState);

  readHeaterState(MAX6675_CS_brew, MAX6675_SO_brew, MAX6675_SCK, &brewHeaterState);  

  readHeaterState(MAX6675_CS_steam, MAX6675_SO_steam, MAX6675_SCK, &steamHeaterState);  

  // Determine next Gaggia state...
  // (e.g. move to 'Done Brewing' state once target weight is achieved, etc.)
  GaggiaState nextGaggiaState = getNextGaggiaState(currentGaggiaState, 
                                                   brewHeaterState, 
                                                   steamHeaterState, 
                                                   scaleState, 
                                                   userInputState);

  if (nextGaggiaState.state != currentGaggiaState.state) {

    // Perform actions given current Gaggia state and input ...
    // This step does also mutate current state
    // (e.g. record weight of beans, tare measuring cup)

    // Things we do when we leave a state
    processOutgoingGaggiaState(currentGaggiaState,
                               nextGaggiaState,
                               &brewHeaterState, 
                               &steamHeaterState, 
                               &scaleState,
                               &displayState,
                               nowTimeMillis);
  }

  currentGaggiaState = nextGaggiaState;

    // Things we do when we enter a state
  processCurrentGaggiaState(currentGaggiaState,
                            &brewHeaterState, 
                            &steamHeaterState, 
                            &scaleState,
                            &displayState,
                            nowTimeMillis);

  firstLoop = false;

  delay(200);
}

void readScaleState(NAU7802 myScale, ScaleState *scaleState) {
  scaleState->measuredWeight = 0;

  if (myScale.available() == true) {
    int reading = myScale.getReading();
    Log.error("reading: " + String(reading));

    int scaledReading = reading*SCALE_FACTOR;

    if (scaledReading > SCALE_OFFSET) {
      scaledReading -= SCALE_OFFSET;
    }

    Log.error("scaledReading: " + String(scaledReading));

    scaleState->measuredWeight = scaledReading;
  }
}

boolean isButtonPressedRightNow() {
  return digitalRead(BUTTON) == HIGH;
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
      Log.error("LONG PRESS!");
      return;
    }
  } else {
    if (userInputState->buttonPressStartTimeMillis > 0) {  
      // falling edge detected...
      if (nowTimeMillis - userInputState->buttonPressStartTimeMillis < BUTTON_LONG_PRESS_DURATION_MILLIS) {
        Log.error("SHORT PRESS!");
        userInputState->state = SHORT_PRESS;
        userInputState->buttonPressStartTimeMillis = -1;

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
  } else {

    // The lower three bits (0,1,2) are discarded status bits
    measuredValue >>= 3;

    // The remaining bits are the number of 0.25 degree (C) counts
    heaterState->measuredTemp = measuredValue*0.25;
  }
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

void startDispensingWater() {
  digitalWrite(WATER_DISPENSER, HIGH);
}

void stopDispensingWater() {
  digitalWrite(WATER_DISPENSER, LOW);
}

// If no decoding is necessary, the original string
// is returned
String decodeMessageIfNecessary(char* _message, 
                                char* escapeSequence,
                                long firstValue,
                                long secondValue,                         
                                char* units,
                                HeaterState *brewHeaterState,
                                HeaterState *steamHeaterState,
                                ScaleState *scaleState) {
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
                        HeaterState *brewHeaterState,
                        HeaterState *steamHeaterState,
                        ScaleState *scaleState,
                        String previousLineDisplayed) {

  display.setCursor(0,line-1);
  
  String lineToDisplay; 
  
  lineToDisplay = decodeMessageIfNecessary(message,
                                           "{adjustedWeight}/{targetBeanWeight}",
                                           filterLowWeight(scaleState->measuredWeight - scaleState->tareWeight),
                                           TARGET_BEAN_WEIGHT,
                                           " g",
                                           brewHeaterState,
                                           steamHeaterState,
                                           scaleState);
  if (lineToDisplay == "") {
    lineToDisplay = decodeMessageIfNecessary(message,
                                            "{measuredBrewTemp}/{targetBrewTemp}",
                                            brewHeaterState->measuredTemp,
                                            TARGET_BREW_TEMP,
                                            " °C",
                                            brewHeaterState,
                                            steamHeaterState,
                                            scaleState);
    if (lineToDisplay == "") {
      lineToDisplay = decodeMessageIfNecessary(message,
                                              "{adjustedWeight}/{targetBrewWeight}",
                                              filterLowWeight(scaleState->measuredWeight - scaleState->tareWeight),
                                              scaleState->targetWeight,
                                              " g",
                                              brewHeaterState,
                                              steamHeaterState,
                                              scaleState);
      if (lineToDisplay == "") {
        lineToDisplay = decodeMessageIfNecessary(message,
                                              "{measuredSteamTemp}/{targetSteamTemp}",
                                              steamHeaterState->measuredTemp,
                                              TARGET_STEAM_TEMP,
                                              " °C",
                                              brewHeaterState,
                                              steamHeaterState,
                                              scaleState);
        if (lineToDisplay == "") {
          lineToDisplay = decodeMessageIfNecessary(message,
                                                "{measuredWeight}",
                                                filterLowWeight(scaleState->measuredWeight - scaleState->tareWeight),                                                0,
                                                " g",
                                                brewHeaterState,
                                                steamHeaterState,
                                                scaleState);
        }      
      }
    }
  }

  if (lineToDisplay == "") {
    lineToDisplay = String(message);
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

boolean shouldTurnOnHeater(float targetTemp,
                           float nowTimeMillis,
                           HeaterState *heaterState) {
                        
  boolean shouldTurnOnHeater = false;

  if (heaterState->heaterStarTime > 0 ) {
    // we're in a heat cycle....

    if ((nowTimeMillis - heaterState->heaterStarTime) > heaterState->heaterDurationMillis) {
      // done this heat cycle!

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
      heaterState->heaterStarTime = nowTimeMillis;

      shouldTurnOnHeater = true;
    }
  }

  return shouldTurnOnHeater;
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

GaggiaState getNextGaggiaState(GaggiaState currentGaggiaState, 
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

    case TARE_CUP_2 :

      if (userInputState.state == SHORT_PRESS) {
        return heatingToBrewState;
      }
      if (userInputState.state == LONG_PRESS) {
        return helloState;
      }
      break;

    case HEATING_TO_BREW :

      if (brewHeaterState.measuredTemp >= TARGET_BREW_TEMP) {
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

      if (userInputState.state == SHORT_PRESS) {
        return helloState;
      }
      if (userInputState.state == LONG_PRESS) {
        return helloState;
      }
      break;

    case HEATING_TO_STEAM :

      if (steamHeaterState.measuredTemp >= TARGET_STEAM_TEMP) {
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
      
    case COOL_START :

      if (userInputState.state == SHORT_PRESS) {
        return coolingState;
      }
      if (userInputState.state == LONG_PRESS) {
        return helloState;
      }
      break;

    case COOLING :

      if (brewHeaterState.measuredTemp <= TARGET_BREW_TEMP) {
        return coolDoneState;
      }
      if (userInputState.state == LONG_PRESS) {
        return helloState;
      }
      break;

    case COOL_DONE :

      if (userInputState.state == SHORT_PRESS) {
        return helloState;
      }
      if (userInputState.state == LONG_PRESS) {
        return helloState;
      }
      break;  
  } 

  return currentGaggiaState;
}  

void processOutgoingGaggiaState(GaggiaState currentGaggiaState,  
                                GaggiaState nextGaggiaState,
                                HeaterState *brewHeaterState,
                                HeaterState *steamHeaterState,
                                ScaleState *scaleState,
                                DisplayState *displayState,
                                float nowTimeMillis) {

  // Process Tare Scale
  if (currentGaggiaState.tareScale) {
    scaleState->tareWeight = scaleState->measuredWeight;
  }

  // Process Record Weight 
  if (currentGaggiaState.recordWeight) {
    scaleState->targetWeight = 
      (scaleState->measuredWeight - scaleState->tareWeight)*BREW_MEASURED_WEIGHT_MULTIPLIER; 
  }
}

void processCurrentGaggiaState(GaggiaState currentGaggiaState,  
                               HeaterState *brewHeaterState,
                               HeaterState *steamHeaterState,
                               ScaleState *scaleState,
                               DisplayState *displayState,
                               float nowTimeMillis) {
  // 
  // Process Display
  //
  displayState->display1 = updateDisplayLine(currentGaggiaState.display1, 
                                             1, 
                                             brewHeaterState, 
                                             steamHeaterState, 
                                             scaleState, 
                                             displayState->display1);

  displayState->display2 = updateDisplayLine(currentGaggiaState.display2, 
                                             2, 
                                             brewHeaterState, 
                                             steamHeaterState, 
                                             scaleState, 
                                             displayState->display2);

  displayState->display3 = updateDisplayLine(currentGaggiaState.display3, 
                                             3, 
                                             brewHeaterState, 
                                             steamHeaterState, 
                                             scaleState,  
                                             displayState->display3);

  displayState->display4 = updateDisplayLine(currentGaggiaState.display4, 
                                             4, 
                                             brewHeaterState, 
                                             steamHeaterState, 
                                             scaleState, 
                                             displayState->display4);



  // Process Heaters
  //
  // Even though a heater may be 'on' during this state, the control system
  // for the heater turns it off and on intermittently in an attempt to regulate
  // the temperature around the target temp.
  if (currentGaggiaState.brewHeaterOn) {
    if (shouldTurnOnHeater(TARGET_BREW_TEMP, nowTimeMillis, brewHeaterState)) {
      turnBrewHeaterOn();
    } else {
      turnBrewHeaterOff();
    } 
  } else {
    turnBrewHeaterOff();
  }

  if (currentGaggiaState.steamHeaterOn) {
    if (shouldTurnOnHeater(TARGET_STEAM_TEMP, nowTimeMillis, steamHeaterState)) {
      turnSteamHeaterOn();
    } else {
      turnSteamHeaterOff();
    } 
  } else {
    turnSteamHeaterOff();
  }

  // Process Dispense Water 
  if (currentGaggiaState.dispenseWater) {
    startDispensingWater();
  } else {
    stopDispensingWater();
  }
}