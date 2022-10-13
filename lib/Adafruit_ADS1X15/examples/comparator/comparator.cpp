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
    
     // Setup 3V comparator on channel 0
    ads.startComparator_SingleEnded(0, 16000);
    
    Serial.println("Getting single-ended reading from AIN0 with >3.0V comparator");
    Serial.println("ADC Range: +/- 6.144V (1 bit = 0.1875mV for ADS1115)");
    Serial.println("Comparator Threshold: 16000 (3.000V)");
    

    //Setup D2 as INPUT and Connect ALERT from ADS1115 to Photon D2
    pinMode(D2, INPUT);
    pinMode(D7, OUTPUT);
}

void loop(void) 
{
    double multiplier = 0.1875F; //milli Volts per bit for ADS1115
    //double multiplier = 3.0F; //milli Volts per bit for ADS1105
    
    // Comparator will only de-assert after a read
    short adc0 = ads.getLastConversionResults();
    double av0 = adc0 * multiplier;
    
    Serial.print("AIN0: "); Serial.print(adc0); Serial.print(", AV0: "); Serial.print(av0,7); Serial.println("mV");
    
    //If comparator ALERT is pulled LOW, SET D7 LED HIGH
    if (!digitalRead(D2))
        digitalWrite(D7, HIGH);
        
    delay(1000);
}