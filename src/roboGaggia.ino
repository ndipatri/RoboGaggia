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
#define BUTTON  D3


//
// Contants
//

// These are constants of proportionality for the three PID 
// components: current (p), past (i), and future (d)
int kp = 9.1;   
int ki = 0.3;   
int kd = 1.8;

//float targetBrewTempC = 103;
//float targetSteamTempC = 140;

// NJD TODO - for testing
float targetBrewTemp = 45;
float targetSteamTemp = 55;


//
// State
//

// The difference between current temperature and set temperature
float previousBrewTempError = 0;
float previousSteamTempError = 0;

// Used for heater duration calculation
float previousBrewSampleTime;
float previousSteamSampleTime;

// Used to track ongoing brew/steam heat cycles...
float brewHeaterStarTime = -1;
float brewHeaterDurationMillis = -1;

float steamHeaterStartTime = -1;
float steamHeaterDurationMillis = -1;

bool steamHeaterShouldBeOn = false;
bool brewHeaterShouldBeOn = true;


// Not necessary to store, but making available as Particle Variable.
double measuredBrewTemp;
double measuredSteamTemp;


//
// the state of the button
//

// We need to evaluate the state of a button over time to determine
// if it's a short press or a long press
float previousButtonSampleTime;

// How long the button has to be pressed before it is considered
// a long press
float buttonLongPressDurationMillis = -1;

enum ButtonState {
  IDLE = 0,
  SHORT_PRESS = 1,
  LONG_PRESS = 2 
};
ButtonState currentButtonState = IDLE;



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
  
  previousBrewSampleTime = millis(); 
  previousSteamSampleTime = millis(); 
  previousButtonSampleTime = millis(); 
  
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
  pinMode(BUTTON, INPUT);

  Particle.variable("measuredBrewTemp", measuredBrewTemp);
  Particle.variable("measuredSteamTemp", measuredSteamTemp);

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

void loop() {
  
  // NJD TODO - For now, we will do the brew and steam heaters sequentially until we get 
  // everything tuned.. the problem with this approach is it blocks this loop thread
  // so we can't listen for input.. evnentually we will need to 'schedule' the start
  // and stop brew/steam intervals.. and make the loop interval MUCH faster (e.g. 100ms)..
  // In one loop we would start a heater and in a subsequent loop we would end the heater duty
  // cycle...
  
  //
  // Check on Brew Heater State
  //
  // If the Gaggia is on, we're always trying to set the
  // brew temp!
  //
  float now = millis();  

  measuredBrewTemp = measureTemperature(MAX6675_CS_brew, MAX6675_SO_brew, MAX6675_SCK);  
  Log.error("measuredBrewTempC '" + String(measuredBrewTemp) + "'.");

  measuredSteamTemp = measureTemperature(MAX6675_CS_steam, MAX6675_SO_steam, MAX6675_SCK);  
  Log.error("measuredSteamTempC '" + String(measuredSteamTemp) + "'.");

  long measuredWeight = 0;
  if(myScale.available() == true) {
    measuredWeight = myScale.getReading();
    Log.error("Scale Reading: " + String(measuredWeight));
  }



  if (previousButtonSampleTime > 0 ) {
      // we're in a button cycle.... meaning
      // in the near past, the button was pressed... our job here
      // is to determine if it's a short press or a long press

      if ((now - previousButtonSampleTime) > brewHeaterDurationMillis) {
        // done this brew heat cycle!
        Log.error("Stopping current brew heat cycle.");
        turnBrewHeaterOff();
        brewHeaterStarTime = -1;
      }
    } else {
      // determine if we should be in a brew heat cycle ...

      brewHeaterDurationMillis = calculateHeaterPulseDurationMillis(measuredBrewTemp, targetBrewTemp, &previousBrewTempError, &previousBrewSampleTime);
      if (brewHeaterDurationMillis > 0) {
        Log.error("Starting brew heat cycle for '" + String(brewHeaterDurationMillis) + "' milliseconds.");
        brewHeaterStarTime = now;
        turnBrewHeaterOn();
      }
    }






  if (brewHeaterShouldBeOn) {
    if (brewHeaterStarTime > 0 ) {
      // we're in a brew heat cycle....

      if ((now - brewHeaterStarTime) > brewHeaterDurationMillis) {
        // done this brew heat cycle!
        Log.error("Stopping current brew heat cycle.");
        turnBrewHeaterOff();
        brewHeaterStarTime = -1;
      }
    } else {
      // determine if we should be in a brew heat cycle ...

      brewHeaterDurationMillis = calculateHeaterPulseDurationMillis(measuredBrewTemp, targetBrewTemp, &previousBrewTempError, &previousBrewSampleTime);
      if (brewHeaterDurationMillis > 0) {
        Log.error("Starting brew heat cycle for '" + String(brewHeaterDurationMillis) + "' milliseconds.");
        brewHeaterStarTime = now;
        turnBrewHeaterOn();
      }
    }
  } else {
    turnBrewHeaterOff();
  }


  //
  // Check on Stem Heater State
  //
  // We only steam if user has chosen to do so...
  //
  //

  if (steamHeaterShouldBeOn) {
    if (steamHeaterStartTime > 0 ) {
      // we're in a steam heat cycle....

      if ((now - steamHeaterStartTime) > steamHeaterDurationMillis) {
        // done this steam heat cycle!
        Log.error("Stopping current steam heat cycle.");
        turnSteamHeaterOff();
        steamHeaterStartTime = -1;
      }

    } else {
      // determine if we should be in a steam heat cycle ...

      steamHeaterDurationMillis = calculateHeaterPulseDurationMillis(measuredSteamTemp, targetSteamTemp, &previousSteamTempError, &previousSteamSampleTime);
      if (steamHeaterDurationMillis > 0) {
        Log.error("Starting steam heat cycle for '" + String(steamHeaterDurationMillis) + "' milliseconds.");
        steamHeaterStartTime = now;
        turnSteamHeaterOn();
      }
    }  
  } else {
    turnSteamHeaterOff();
  }





  lcd.clear();
  updateDisplayLine("Hello", 1);
  updateDisplayLine("Temp : " + String(measuredBrewTemp), 2);
  updateDisplayLine("Weight : " + String(measuredWeight), 3);

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

// line starts at 1
void updateDisplayLine(const char* message, int line) {
  lcd.setCursor(0,line-1);
  lcd.print(message);
}

double measureTemperature(int CHIP_SELECT_PIN, int SERIAL_OUT_PIN, int SERIAL_CLOCK_PIN) {

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

  if (measuredValue & 0x4) 
  {    
    // Bit 2 indicates if the thermocouple is disconnected
    return NAN;     
  }

  // The lower three bits (0,1,2) are discarded status bits
  measuredValue >>= 3;

  // The remaining bits are the number of 0.25 degree (C) counts
  return measuredValue*0.25;
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