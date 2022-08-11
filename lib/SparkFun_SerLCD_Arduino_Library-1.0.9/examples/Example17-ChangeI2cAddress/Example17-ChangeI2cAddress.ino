/*
  SparkFun Electronics
  SerLCD Library - Change I2C address
  Pete Lewis - August 18, 2020

  This example demonstrates how to change the i2c address on your LCD.
  Note, once you change the address, then you will need to call ".begin()" again.

  There is a set range of available addresses from 0x07 to 0x78, so make sure your
  chosen address falls within this range.

  The next thing to note is that when you change the address you'll
  need to call .begin again to talk to that screen.

  Finally if for some reason you've forgotten your new address. No big deal, run a
  hardware reset on your screen to get it back to the default address (0x72).
  To cause a hardware reset, simply tie the RX pin LOW, and they cycle power
  (while continuing to hold RX low). Then release RX, and cycle power again.

  The circuit:
   SparkFun RGB OpenLCD Serial display connected through
   a SparkFun Qwiic cable/adpater to an qwiic-enabled Arduino.

  The Qwiic adapter should be attached to the display as follows:
  Display	/ Qwiic Cable Color
 	GND	/	Black
 	RAW	/	Red
 	SDA	/	Blue
 	SCL	/	Yellow

  Note: If you connect directly to a 5V Arduino instead, you *MUST* use
  a level-shifter to convert the i2c voltage levels down to 3.3V for the display.

  This code is based on the LiquidCrystal code originally by David A. Mellis
  and the OpenLCD code by Nathan Seidle at SparkFun.

  Also based off the original Arduino Library code with many contributions from
  Gaston Williams - August 29, 2018

  Some code/comments/ideas ported from the Qwiic Quad Relay Arduino Library
  Written by Elias Santistevan, July 2019

  License: This example code is in the public domain.

  More info on Qwiic here: https://www.sparkfun.com/qwiic

  AVR-Based Serial Enabled LCDs Hookup Guide
  https://learn.sparkfun.com/tutorials/avr-based-serial-enabled-lcds-hookup-guide
*/

#include <Wire.h>

#include <SerLCD.h> //Click here to get the library: http://librarymanager/All#SparkFun_SerLCD
SerLCD lcd; // Initialize the library with default I2C address 0x72

byte oldAddress = 0x72; // default 0x72
byte newAddress = 0x71; // must be within 0x07 to 0x78, DEFAULT: 0x72

void setup() {
  Wire.begin();
  Serial.begin(115200);

  Serial.print("Connecting to SerLCD at 0x");
  Serial.println(oldAddress, HEX);
  lcd.begin(Wire, oldAddress); //Set up the LCD for I2C communication
  Serial.println("Done\n\r");

  lcd.setBacklight(255, 255, 255); //Set backlight to bright white
  lcd.setContrast(5); //Set contrast. Lower to 0 for higher contrast.

  lcd.clear(); //Clear the display - this moves the cursor to home position as well

  // command to change address
  // note this will also change class private variable "lcd._i2cAddr"
  Serial.print("Changing address to 0x");
  Serial.println(newAddress, HEX);
  lcd.setAddress(newAddress);
  Serial.println("Done\n\r");
  
  Serial.print("Connecting to SerLCD at 0x");
  Serial.println(newAddress, HEX);
  lcd.begin(Wire); // note, new address argument is not needed. lcd._i2cAddr has been updated by ".setAddress()"
  Serial.println("Done\n\r");

  lcd.print("My new address: 0x"); // print it to the LCD for user victory experience
  lcd.print(lcd.getAddress(), HEX); // note, we need to use public function to access private lcd._i2cAddr
}

void loop() {
  // do nothing
}