#include "Adafruit_ADS1X15.h"

Adafruit_ADS1115 ads;  /* Use this for the 16-bit version */
//Adafruit_ADS1015 ads;     /* Use thi for the 12-bit version */


// CONNECT PHOTON TO ADS1115 BOARD AS FOLLOWS:
//
//  PHOTON PIN  ->  ADS11x5 PIN
//  ----------      -----------
//  3.3V        ->  VDD
//  GND         ->  GND
//  D0          ->  SDA (I2C DATA)
//  D1          ->  SCL (I2C CLOCK)
//  D2          ->  ALERT

void setup(void) 
{
  Serial.begin(9600);
  Serial.println("Setup..");
  
  // The ADC input range (or gain) can be changed via the following
  // functions, but be careful NEVER TO EXCEED +0.3V OVER VDD ON GAINED INPUTS,
  // or exceed the upper and lower limits if you adjust the input range!
  // Setting these values incorrectly may DESTROY your ADC!
  //
  //  *** TAKE CARE WHEN SETTING GAIN TO NOT EXCEED ABOVE LIMITS ON INPUT!!
  //                                                                    ADS1015   ADS1115
  //                                                                    -------   -------
   ads.setGain(GAIN_TWOTHIRDS);  // 2/3x gain +/- 6.144V  1 bit =       3mV       0.1875mV (DEFAULT)
  // ads.setGain(GAIN_ONE);        // 1x gain   +/- 4.096V  1 bit =     2mV       0.125mV
  // ads.setGain(GAIN_TWO);        // 2x gain   +/- 2.048V  1 bit =     1mV       0.0625mV
  // ads.setGain(GAIN_FOUR);       // 4x gain   +/- 1.024V  1 bit =     0.5mV     0.03125mV
  // ads.setGain(GAIN_EIGHT);      // 8x gain   +/- 0.512V  1 bit =     0.25mV    0.015625mV
  // ads.setGain(GAIN_SIXTEEN);    // 16x gain  +/- 0.256V  1 bit =     0.125mV   0.0078125mV  
  ads.begin();
  
  Serial.println("Getting single-ended readings from AIN01,2,3");
  Serial.println("ADC Range: +/- 6.144V (1 bit = 0.1875mV for ADS1115)");

}

void loop(void) 
{
  double multiplier = 0.1875F; //milli Volts per bit for ADS1115
  //double multiplier = 3.0F; //milli Volts per bit for ADS1105

  short adc0, adc1, adc2, adc3;
  double av0, av1, av2, av3;
  
  adc0 = ads.readADC_SingleEnded(0);
  av0 = adc0 * multiplier;
  adc1 = ads.readADC_SingleEnded(1);
  av1 = adc1 * multiplier;
  adc2 = ads.readADC_SingleEnded(2);
  av1 = adc1 * multiplier;
  adc3 = ads.readADC_SingleEnded(3);
  av3 = adc3 * multiplier;
  
  Serial.print("AIN0: "); Serial.print(adc0); Serial.print(", AV0: "); Serial.print(av0,7); Serial.println("mV");
  Serial.print("AIN1: "); Serial.print(adc1); Serial.print(", AV1: "); Serial.print(av1,7); Serial.println("mV");
  Serial.print("AIN2: "); Serial.print(adc2); Serial.print(", AV2: "); Serial.print(av2,7); Serial.println("mV");
  Serial.print("AIN3: "); Serial.print(adc3); Serial.print(", AV3: "); Serial.print(av3,7); Serial.println("mV");
  Serial.println(" ");
  
  delay(1000);
}

