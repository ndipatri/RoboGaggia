/*
  SerLCD Library - display() and noDisplay()
  Gaston Williams - August 29, 2018

  This sketch prints "Hello World!" to the LCD and uses the
  display() and noDisplay() functions to turn on and off
  the display.

  The circuit:
   SparkFun RGB OpenLCD Serial display connected through
   a Sparkfun Qwiic adpater to an Ardruino with a
   Qwiic shield or a Sparkfun Blackboard with Qwiic built in.

  The Qwiic adapter should be attached to the display as follows:
  Display  / Qwiic Cable Color
  GND / Black
  RAW / Red
  SDA / Blue
  SCL / Yellow

  Note: If you connect directly to a 5V Arduino instead, you *MUST* use
  a level-shifter to convert the i2c voltage levels down to 3.3V for the display.

  License: This example code is in the public domain.
*/

#include <Wire.h>

#include <SerLCD.h> //Click here to get the library: http://librarymanager/All#SparkFun_SerLCD
SerLCD lcd; // Initialize the library with default I2C address 0x72

void setup() {
  Wire.begin();
  lcd.begin(Wire);

  Wire.setClock(400000); //Optional - set I2C SCL to High Speed Mode of 400kHz

  // Print a message to the LCD.
  lcd.print("Display on/off example");
}

void loop() {
  // Turn off the display:
  lcd.noDisplay();
  delay(500);
  // Turn on the display:
  lcd.display();
  delay(500);
}

