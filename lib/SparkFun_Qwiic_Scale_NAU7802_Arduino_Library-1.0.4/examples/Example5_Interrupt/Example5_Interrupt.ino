/*
  Use the Qwiic Scale to read load cells and scales
  By: Nathan Seidle @ SparkFun Electronics
  Date: March 3rd, 2019
  License: This code is public domain but you buy me a beer if you use this
  and we meet someday (Beerware license).

  The interrupt pin indicates when a sample has been taken. This example shows 
  how the INT pin can be configured active high or active low.

  SparkFun labored with love to create this code. Feel like supporting open
  source? Buy a board from SparkFun!
  https://www.sparkfun.com/products/15242

  Hardware Connections:
  Plug a Qwiic cable into the Qwiic Scale and a RedBoard Qwiic
  If you don't have a platform with a Qwiic connection use the SparkFun Qwiic Breadboard Jumper (https://www.sparkfun.com/products/14425)
  Open the serial monitor at 9600 baud to see the output
*/

#include <Wire.h>

#include "SparkFun_Qwiic_Scale_NAU7802_Arduino_Library.h" // Click here to get the library: http://librarymanager/All#SparkFun_NAU8702

NAU7802 myScale; //Create instance of the NAU7802 class

byte interruptPin = 2; //Tied to the INT pin on Qwiic Scale. Can be any pin.

void setup()
{
  pinMode(interruptPin, INPUT); //No need for pullup. NAU7802 does not use open drain INT.

  Serial.begin(9600);
  Serial.println("Qwiic Scale Example");

  Wire.begin();

  if (myScale.begin() == false)
  {
    Serial.println("Scale not detected. Please check wiring. Freezing...");
    while (1);
  }
  Serial.println("Scale detected!");

  //myScale.setIntPolarityHigh(); //Set Int pin to be high when data is ready (default)
  myScale.setIntPolarityLow(); //Set Int pin to be low when data is ready
}

void loop()
{
  if (digitalRead(interruptPin) == LOW)
  {
    long currentReading = myScale.getReading();
    Serial.print("Reading: ");
    Serial.println(currentReading);
  }
}
