/*
  SerLCD Library - Backlight
  Gaston Williams - August 29, 2018

  This sketch changes the backlight color and displays text using
  the OpenLCD functions. This works with the original version of 
  SerLCD. See FastBacklight example for version 1.1 and later.

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

  //By default .begin() will set I2C SCL to Standard Speed mode of 100kHz
  Wire.setClock(400000); //Optional - set I2C SCL to High Speed Mode of 400kHz
}

void loop() {
  lcd.setBacklight(0, 0, 0); //black is off
  lcd.clear(); //Clear the display - this moves the cursor to home position as well
  lcd.print("Black (off)");
  delay(3000);
  
  lcd.setBacklight(255, 0, 0); //bright red
  lcd.clear();
  lcd.print("Red");
  delay(3000);
  
  lcd.setBacklight(0xFF8C00); //orange
  lcd.clear();
  lcd.print("Orange");
  delay(3000);
  
  lcd.setBacklight(255, 255, 0); //bright yellow
  lcd.clear();
  lcd.print("Yellow");
  delay(3000);
  
  lcd.setBacklight(0, 255, 0); //bright green
  lcd.clear();
  lcd.print("Green");
  delay(3000);
  
  lcd.setBacklight(0, 0, 255); //bright blue
  lcd.clear();
  lcd.print("Blue");
  delay(3000);
  
  lcd.setBacklight(0x4B0082); //indigo, a kind of dark purplish blue
  lcd.clear();
  lcd.print("Indigo");
  delay(3000);
  
  lcd.setBacklight(0xA020F0); //violet
  lcd.clear();
  lcd.print("Violet");
  delay(3000);
  
  lcd.setBacklight(0x808080); //grey
  lcd.clear();
  lcd.print("Grey");
  delay(3000);

  lcd.setBacklight(255, 255, 255); //bright white
  lcd.clear();
  lcd.print("White");
  delay(3000);
}
