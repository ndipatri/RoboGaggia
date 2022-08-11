/*
 *  This is example code that shows how to send data over SPI to the display.
 *
 * To get this code to work, attached an OpenLCD to an Arduino Uno using the following pins:
 * CS (OpenLCD) to 10 (Arduino)
 * SDI to 11
 * SDO to 12 (optional)
 * SCK to 13
 */

// include the Qwiic OpenLCD library code:
#include <QwiicSerLCD.h>
#include <SPI.h>

byte csPin = 10; //You can use any output pin but for this example we use 10


// initialize the library
QwiicSerLCD lcd;

void setup() {
  pinMode(csPin, OUTPUT);
  SPI.begin();

  lcd.begin(SPI, csPin, SPISettings(100000, MSBFIRST, SPI_MODE0));
//For Arduino versions before 1.6, use the two lines below instead
//  SPI.setClockDivider(SPI_CLOCK_DIV128); //Slow down the master a bit
//  lcd.begin(SPI, csPin);

  //Clear the display
  lcd.clear();
  
  //Print a message to the LCD.
  lcd.print("Hello, World!");
}

void loop() {
  // set the cursor to column 0, line 1
  // (note: line 1 is the second row, since counting begins with 0):
  lcd.setCursor(0, 1);
  // print the number of seconds since reset:
  lcd.print(millis() / 1000);
}

