#include "Heater.h"
#include <pid.h>

#define HEATER D8

HeaterState heaterState;

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

// This measured temperature assures that the extracted temp
// at the group is around 93C/200F
double TARGET_BREW_TEMP = 103; 

double TARGET_HOT_WATER_DISPENSE_TEMP = 110; 

// This will trigger the COOLING feature
double TOO_HOT_TO_BREW_TEMP = 110; 
double TARGET_STEAM_TEMP = 140; 

// These were emperically derived.  They are highly dependent on the actual system , but should now work
// for any RoboGaggia.
// see https://en.wikipedia.org/wiki/PID_controller#Loop_tuning
double heater_PID_kP = 8.0;
double heater_PID_kI = 5.0;
double heater_PID_kD = 2.0;

void readHeaterState(int CHIP_SELECT_PIN, int SERIAL_OUT_PIN, int SERIAL_CLOCK_PIN) {

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
    heaterState.thermocoupleError = true;

  } else {
    
    heaterState.thermocoupleError = false;

    // The lower three bits (0,1,2) are discarded status bits
    measuredValue >>= 3;

    publishParticleLog("renderHeaterState", "measuredInC:" + String(measuredValue*0.25));

    // The remaining bits are the number of 0.25 degree (C) counts
    heaterState.measuredTemp = measuredValue*0.25;
  }
}

void readBrewHeaterState() {
  readHeaterState(MAX6675_CS_brew, MAX6675_SO_brew, MAX6675_SCK);
}

void readSteamHeaterState() {
  readHeaterState(MAX6675_CS_steam, MAX6675_SO_steam, MAX6675_SCK);
}

boolean shouldTurnOnHeater() {
                        
  // By default, it will update output values every 200ms, regardless
  // of how often we call Compute()
  heaterState.heaterPID->Compute();

  if (heaterState.heaterShouldBeOn > 0 ) {
    return true;
  } else {
    return false;
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

void configureHeater(double *heaterTemp) {
    PID *thisHeaterPID = new PID(&heaterState.measuredTemp, 
                                 &heaterState.heaterShouldBeOn, 
                                 heaterTemp, 
                                 heater_PID_kP, heater_PID_kI, heater_PID_kD, PID::DIRECT);
    // The heater is either on or off, there's no need making this more complicated..
    // So the PID either turns the heater on or off.
    thisHeaterPID->SetOutputLimits(0, 1);
    thisHeaterPID->SetMode(PID::AUTOMATIC);
    thisHeaterPID->SetSampleTime(500);


    delete heaterState.heaterPID;
    heaterState.heaterPID = thisHeaterPID;
}

void configureBrewHeater() {
    configureHeater(&TARGET_BREW_TEMP);
}

void configureSteamHeater() {
    configureHeater(&TARGET_STEAM_TEMP);
}

void configureHotWaterDispenseHeater() {
    configureHeater(&TARGET_HOT_WATER_DISPENSE_TEMP);
}

void heaterInit() {
  
  // setup MAX6675 to read the temperature from thermocouple
  pinMode(MAX6675_CS_brew, OUTPUT);
  pinMode(MAX6675_CS_steam, OUTPUT);
  pinMode(MAX6675_SO_brew, INPUT);
  pinMode(MAX6675_SO_steam, INPUT);
  pinMode(MAX6675_SCK, OUTPUT);
  
  // external heater elements
  pinMode(HEATER, OUTPUT);

  Particle.variable("targetBrewTempC", TARGET_BREW_TEMP);
  Particle.variable("currentBrewTempC", heaterState.measuredTemp);
}