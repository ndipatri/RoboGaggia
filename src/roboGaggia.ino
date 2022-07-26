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

// Chip Select! - tie low to turn on MAX6675 and 
// get a temperature reading
#define MAX6675_CS   D2 

// Serial Clock - when brought high, shifts
// another byte from the MAX6675.
#define MAX6675_SCK  D3

// Serial Data Out
#define MAX6675_SO   D4 

// CS, SCK, and SO together are used to read serial data
// from the MAX6675 

// These are constants of proportionality for the three PID 
// components: current (p), past (i), and future (d)
int kp = 9.1;   
int ki = 0.3;   
int kd = 1.8;

float targetTempC = 200;
float currentTempC = 0.0;

// The difference between current temperature and set temperature
float currentError = 0;
float previousError = 0;

// Used to calcualte historical values (i)
float elapsedTime;
float currentTime;
float previousTime;

// These are calculated based on historic data and
// current temeprature...
int PID_p = 0;    
int PID_i = 0;    
int PID_d = 0;

int currentOutput = 0;


double measuredTemperatureC;

// setup() runs once, when the device is first turned on.
void setup() {
  
  // strange but leaving this for now...
  currentTime = millis(); 
  
  // setup MAX6675 to read the temperature from thermocouple
  pinMode(MAX6675_CS, OUTPUT);
  pinMode(MAX6675_SO, INPUT);
  pinMode(MAX6675_SCK, OUTPUT);

  Particle.variable("measuredTemperatureC1", measuredTemperatureC);

  // Wait for a USB serial connection for up to 15 seconds
  waitFor(Serial.isConnected, 15000);
}

// loop() runs over and over again, as quickly as it can execute.
void loop() {
  measuredTemperatureC = measureTemperatureC();  

  int durationMillis = calculateHeaterPulseDurationMillis(measuredTemperatureC);

  pulseBrewHeaterFor(durationMillis);

  delay(3000);
}

double measureTemperatureC() {

  uint16_t measuredValue;

  // enable MAX6675
  digitalWrite(MAX6675_CS, LOW);
  delay(1);

  // Read in 16 bits,
  //  15    = 0 always
  //  14..2 = 0.25 degree counts MSB First
  //  2     = 1 if thermocouple is open circuit  
  //  1..0  = uninteresting status
  
  measuredValue = shiftIn(MAX6675_SO, MAX6675_SCK, MSBFIRST);
  measuredValue <<= 8;
  measuredValue |= shiftIn(MAX6675_SO, MAX6675_SCK, MSBFIRST);
  
  // disable MAX6675
  digitalWrite(MAX6675_CS, HIGH);

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

int calculateHeaterPulseDurationMillis(double currentTempC) {

  Log.error("currentTempC '" + String(currentTempC) + "'.");

  currentError = targetTempC - currentTempC;

  // Calculate the P value
  PID_p = kp * currentError;

  // Calculate the I value in a range on +-3
  if(-3 < currentError < 3)
  {
    PID_i = PID_i + (ki * currentError);
  }

  //For derivative we need real time to calculate speed change rate
  previousTime = currentTime;                            // the previous time is stored before the actual time read
  currentTime = millis();                            // actual time read
  elapsedTime = (currentTime - previousTime) / 1000; 

  // Now we can calculate the D value - the derivative or slope.. which is a means
  // of predicting future value
  PID_d = kd*((currentError - previousError)/elapsedTime);
  previousError = currentError;     //Remember to store the previous error for next loop.

  
  //vFinal total PID value is the sum of P + I + D
  currentOutput = PID_p + PID_i + PID_d;

  //We define PWM range between 0 and 255
  if(currentOutput < 0)
  {    currentOutput = 0;    }
  if(currentOutput > 255)  
  {    currentOutput = 255;  }

  return currentOutput;
}

void pulseBrewHeaterFor(int headerPulseDurationMillis) {
  Log.error("Pulsing Brew Heater for '" + String(headerPulseDurationMillis) + "' milliseconds.");
}