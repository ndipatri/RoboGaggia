/*
  QwiicSerLCD Library - Hello World

 Demonstrates the use of a Sparkfun AVR-Based Serial Enabled LCD
 display with a Qwiic adapter.

 This sketch prints "Hello World!" to the LCD
 and shows the time.

 The circuit:
 * Sparkfun RGB OpenLCD Serial display connected through 
 * a Sparkfun Qwiic adpater to an Ardruino with a 
 * Qwiic shield or a Sparkfun Blackboard with Qwiic built in.
 * 
 * The Qwiic adapter should be attached to the display as follows:
 *	
 * 	Display	Qwiic	Qwiic Cable Color
 *	GND	GND	Black
 *	RAW	3.3V	Red
 *	SDA	SDA	Blue
 *	SCL	SCL	Yellow
 *
 * Note: If you connect directly to a 5V Arduino instead, you *MUST* use
 * a level-shifter to convert the i2c voltage levels down to 3.3V for the display.
 

 Based on the LiquidCrystal code originally by David A. Mellis
 and the OpenLCD code by Nathan Seidle at Sparkfun.
 
 modified 5 Jul 2009
 by Limor Fried (http://www.ladyada.net)
 example added 9 Jul 2009
 by Tom Igoe
 modified 22 Nov 2010
 by Tom Igoe
 modified 7 Nov 2016
 by Arturo Guadalupi
 modified 29 Aug 2018
 by Gaston Williams

 This example code is in the public domain.
 http://www.arduino.cc/en/Tutorial/LiquidCrystalHelloWorld
 
 More info on Qwiic here:
 https://www.sparkfun.com/qwiic
 
 AVR-Based Serial Enabled LCDs Hookup Guide
 https://learn.sparkfun.com/tutorials/avr-based-serial-enabled-lcds-hookup-guide 
*/

// include the Qwiic OpenLCD library code:
#include <QwiicSerLCD.h>
#include <SoftwareSerial.h>

SoftwareSerial mySerial(6, 7); //RX, TX

// initialize the library
QwiicSerLCD lcd;

void setup() {
  mySerial.begin(9600); //Start communication with OpenLCD

  lcd.begin(mySerial);
  // Print a message to the LCD.
  lcd.print("Hello, World!");
}

void loop() {
  // set the cursor to column 0, line 1
  // (note: line 1 is the second row, since counting begins with 0):
  lcd.setCursor(0, 1);
  // print the number of seconds since reset:
  lcd.print(millis() / 1000);
}

