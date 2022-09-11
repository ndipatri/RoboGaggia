/*
  Use the Qwiic Scale to read load cells and scales
  By: Nathan Seidle @ SparkFun Electronics
  Date: March 3rd, 2019
  License: This code is public domain but you buy me a beer if you use this 
  and we meet someday (Beerware license).

  The NAU7802 runs at ~2mA but can be powered down to 200nA if needed.
  This example turns the scale on once per second.
  
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

void setup()
{
  Serial.begin(9600);
  Serial.println("Qwiic Scale Example");

  Wire.begin();

  if (myScale.begin() == false)
  {
    Serial.println("Scale not detected. Please check wiring. Freezing...");
    while (1);
  }
  Serial.println("Scale detected!");
}

void loop()
{
  myScale.powerDown(); //Power down to ~200nA
  delay(1000);
  myScale.powerUp(); //Power up scale. This scale takes ~600ms to boot and take reading.

  //Time how long it takes for scale to take a reading
  long startTime = millis();
  while(myScale.available() == false)
    delay(1);
  
  long currentReading = myScale.getReading();
  Serial.print("Startup time: ");
  Serial.print(millis() - startTime);
  Serial.print(", ");
  Serial.println(currentReading);
}
