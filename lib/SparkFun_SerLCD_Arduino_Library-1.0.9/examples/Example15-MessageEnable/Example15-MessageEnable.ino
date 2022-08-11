/*
  SerLCD Library - Control when system message are displayed
  Nathan Seidle - February 16, 2019

  This sketch demonstrates how to turn off the system messages displayed when
  the user changes a setting. For instance 'Contrast: 5' or 'UART: 57600' is
  no longer displays.

  Note - This example and the disableSystemMessages and enableSystemMessages() 
  commands are only supported on SerLCD v1.2 and above.

  The circuit:
   SparkFun RGB OpenLCD Serial display connected through
   a SparkFun Qwiic adpater to an Ardruino with a
   Qwiic shield or a SparkFun Blackboard with Qwiic built in.

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

  License: This example code is in the public domain.

  More info on Qwiic here: https://www.sparkfun.com/qwiic

  AVR-Based Serial Enabled LCDs Hookup Guide
  https://learn.sparkfun.com/tutorials/avr-based-serial-enabled-lcds-hookup-guide
*/

#include <Wire.h>

#include <SerLCD.h> //Click here to get the library: http://librarymanager/All#SparkFun_SerLCD
SerLCD lcd; // Initialize the library with default I2C address 0x72

void setup() {
  Wire.begin();

  lcd.begin(Wire); //Set up the LCD for I2C communication

  lcd.setBacklight(255, 255, 255); //Set backlight to bright white
  lcd.setContrast(5); //Set contrast. Lower to 0 for higher contrast.

  lcd.clear(); //Clear the display - this moves the cursor to home position as well
  lcd.print("Hello, World!");

  lcd.disableSystemMessages(); //Now whenever you change a system setting like UART, or Contrast
  //SerLCD will not display the setting. This makes changing the setting faster, and also
  //invisible to the user.

  //lcd.enableSystemMessages(); //This will re-enable the printing of system messages

  lcd.setContrast(0);
}

void loop() {
  // Set the cursor to column 0, line 1
  // (note: line 1 is the second row, since counting begins with 0):
  lcd.setCursor(0, 1);
  // Print the number of seconds since reset:
  lcd.print(millis() / 1000);
}
