/*
 * Project roboGaggia
 * Description:
 * Author:
 * Date:
 */

#include <Wire.h> 
#include <SPI.h>
 
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


//
// Contants
//

// These are constants of proportionality for the three PID 
// components: current (p), past (i), and future (d)
int kp = 9.1;   
int ki = 0.3;   
int kd = 1.8;

float targetBrewTempC = 103;
float targetSteamTempC = 140;


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

// Used to indicate if user wants to steam milk
bool shouldSteam = false;


// Not necessary to store, but making available as Particle Variable.
double measuredBrewTempC;
double measuredSteamTempC;



// setup() runs once, when the device is first turned on.
void setup() {
  
  previousBrewSampleTime = millis(); 
  previousSteamSampleTime = millis(); 
  
  // setup MAX6675 to read the temperature from thermocouple
  pinMode(MAX6675_CS_brew, OUTPUT);
  pinMode(MAX6675_CS_steam, OUTPUT);
  pinMode(MAX6675_SO_brew, INPUT);
  pinMode(MAX6675_SO_steam, INPUT);
  pinMode(MAX6675_SCK, OUTPUT);
  
  // external heater elements
  pinMode(HEATER_brew, OUTPUT);
  pinMode(HEATER_steam, OUTPUT);


  Particle.variable("measuredBrewTemp", measuredBrewTempC);
  Particle.variable("measuredSteamTemp", measuredSteamTempC);

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

  if (brewHeaterStarTime > 0 ) {
    // we're in a brew heat cycle....

    if ((now - brewHeaterStarTime) > brewHeaterDurationMillis) {
      // done this brew heat cycle!
      Log.error("Stopping current brew heat cycle.");
      digitalWrite(HEATER_brew, LOW);
      brewHeaterStarTime = -1;
    }
  } else {
    // determine if we should be in a brew heat cycle ...

    measuredBrewTempC = measureTemperatureC(MAX6675_CS_brew, MAX6675_SO_brew, MAX6675_SCK);  
    Log.error("measuredBrewTempC '" + String(measuredBrewTempC) + "'.");

    brewHeaterDurationMillis = calculateHeaterPulseDurationMillis(measuredBrewTempC, targetBrewTempC, &previousBrewTempError, &previousBrewSampleTime);
    if (brewHeaterDurationMillis > 0) {
      Log.error("Starting brew heat cycle for '" + String(brewHeaterDurationMillis) + "' milliseconds.");
      brewHeaterStarTime = now;
      digitalWrite(HEATER_brew, HIGH);
    }
  }


  //
  // Check on Stem Heater State
  //
  // We only steam if user has chosen to do so...
  //
  //

  if (shouldSteam) {
    if (steamHeaterStartTime > 0 ) {
      // we're in a steam heat cycle....

      if ((now - steamHeaterStartTime) > steamHeaterDurationMillis) {
        // done this steam heat cycle!
        Log.error("Stopping current steam heat cycle.");
        digitalWrite(HEATER_steam, LOW);
        steamHeaterStartTime = -1;
      }

    } else {
      // determine if we should be in a steam heat cycle ...

      measuredSteamTempC = measureTemperatureC(MAX6675_CS_steam, MAX6675_SO_steam, MAX6675_SCK);  
      Log.error("measuredSteamTempC '" + String(measuredSteamTempC) + "'.");

      steamHeaterDurationMillis = calculateHeaterPulseDurationMillis(measuredSteamTempC, targetSteamTempC, &previousSteamTempError, &previousSteamSampleTime);
      if (steamHeaterDurationMillis > 0) {
        Log.error("Starting steam heat cycle for '" + String(steamHeaterDurationMillis) + "' milliseconds.");
        steamHeaterStartTime = now;
        digitalWrite(HEATER_steam, HIGH);
      }
    }  
  }

  delay(10000);
}

double measureTemperatureC(int CHIP_SELECT_PIN, int SERIAL_OUT_PIN, int SERIAL_CLOCK_PIN) {

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
  if(-3 < currentTempError < 3)
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