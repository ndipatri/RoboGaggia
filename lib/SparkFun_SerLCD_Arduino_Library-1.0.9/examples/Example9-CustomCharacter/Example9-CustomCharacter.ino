/*
  QwiicSerLCD Library - Custom Characters
  Gaston Williams - August 29, 2018

  This sketch prints "I <heart> SerLCD!" and a little dancing man
  to the LCD. Since the Serial OpenLCD display uses a serial command
  to display a customer character, the writeChar() method was added
  for this function.

  Custom characters are recorded to SerLCD and are remembered even after power is lost.
  There is a maximum of 8 custom characters that can be recorded.

  Based on Adafruit's example at
  https://github.com/adafruit/SPI_VFD/blob/master/examples/createChar/createChar.pde

  This example code is in the public domain.
  http://www.arduino.cc/en/Tutorial/LiquidCrystalCustomCharacter

  Also useful:
  http://icontexto.com/charactercreator/

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
*/

#include <Wire.h>

#include <SerLCD.h> //Click here to get the library: http://librarymanager/All#SparkFun_SerLCD
SerLCD lcd; // Initialize the library with default I2C address 0x72

// make some custom characters:
byte heart[8] = {
  0b00000,
  0b01010,
  0b11111,
  0b11111,
  0b11111,
  0b01110,
  0b00100,
  0b00000
};

byte smiley[8] = {
  0b00000,
  0b00000,
  0b01010,
  0b00000,
  0b00000,
  0b10001,
  0b01110,
  0b00000
};

byte frownie[8] = {
  0b00000,
  0b00000,
  0b01010,
  0b00000,
  0b00000,
  0b00000,
  0b01110,
  0b10001
};

byte armsDown[8] = {
  0b00100,
  0b01010,
  0b00100,
  0b00100,
  0b01110,
  0b10101,
  0b00100,
  0b01010
};

byte armsUp[8] = {
  0b00100,
  0b01010,
  0b00100,
  0b10101,
  0b01110,
  0b00100,
  0b00100,
  0b01010
};

void setup() {
  Wire.begin();
  lcd.begin(Wire);

  //Send custom characters to display
  //These are recorded to SerLCD and are remembered even after power is lost
  //There is a maximum of 8 custom characters that can be recorded
  lcd.createChar(0, heart);
  lcd.createChar(1, smiley);
  lcd.createChar(2, frownie);
  lcd.createChar(3, armsDown);
  lcd.createChar(4, armsUp);

  // set the cursor to the top left
  lcd.setCursor(0, 0);

  // Print a message to the LCD.
  lcd.print("I ");
  lcd.writeChar(0); // Print the heart character. We have to use writeChar since it's a serial display.
  lcd.print(" SerLCD! ");
  lcd.writeChar(1); // Print smiley
}

void loop() {
  int delayTime = 200;

  lcd.setCursor(4, 1); //Column, row
  lcd.writeChar(3); // Print the little man, arms down
  delay(delayTime);

  lcd.setCursor(4, 1);
  lcd.writeChar(4); // Print the little man, arms up
  delay(delayTime);
  
}
