/*
  SerLCD Library - Move Cursor
  Gaston Williams - August 29, 2018

  This sketch displays text and then moves the cursor back and forth.  These
  functions are not usually part of the LiquidCrystal library, but the functions
  are available in the Serial OpenLCD display, so I created functions for it.

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

  lcd.clear();
  lcd.cursor(); //Turn on the underline cursor
  lcd.print("Watch the cursor!");
}

void loop() {
  //move cursor left in 3 steps
  lcd.moveCursorLeft();
  delay(500);
  lcd.moveCursorLeft();
  delay(500);
  lcd.moveCursorLeft();
  delay(500);

  //move back in one step
  lcd.moveCursorRight(3);
  delay(500);

  //move cursor ahead 3 places in 1 step
  lcd.moveCursorRight(3);
  delay(500);

  //move back in 3 steps
  lcd.moveCursorLeft();
  delay(500);
  lcd.moveCursorLeft();
  delay(500);
  lcd.moveCursorLeft();
  delay(500);
} //loop
