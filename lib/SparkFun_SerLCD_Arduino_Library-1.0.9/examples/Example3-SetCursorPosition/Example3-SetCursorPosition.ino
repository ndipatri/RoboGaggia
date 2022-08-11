/*
  SerLCD Library - Set Cursor
  Gaston Williams - August 29, 2018

  This sketch randomly picks a cursor position, goes to
  that position using the setCursor() method, and prints a character
  
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

// These constants won't change. But you can change the size of
// your LCD using them:
const int numRows = 2;
//const int numRows = 4;
const int numCols = 16;
//const int numCols = 20;

char thisLetter = 'a';

void setup() {
  Wire.begin();
  lcd.begin(Wire);

  Wire.setClock(400000); //Optional - set I2C SCL to High Speed Mode of 400kHz

  randomSeed(analogRead(A0)); //Feed the random number generator
}

void loop() {
  int randomColumn = random(0, numCols);
  int randomRow = random(0, numRows);
  
  // set the cursor position:
  lcd.setCursor(randomColumn, randomRow);
  // print the letter:
  lcd.write(thisLetter);
  delay(200);

  thisLetter++;
  if(thisLetter > 'z') thisLetter = 'a'; //Wrap the variable
}


