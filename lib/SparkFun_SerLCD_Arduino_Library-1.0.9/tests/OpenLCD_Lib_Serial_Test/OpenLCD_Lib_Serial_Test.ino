#include <QwiicSerLCD.h>
#include <SoftwareSerial.h>

SoftwareSerial mySerial(6, 7); //RX, TX

byte counter = 0;
byte contrast = 10;

// initialize the library
QwiicSerLCD lcd;

void setup()
{
  Serial.begin(9600); //Start serial communication at 9600 for debug statements
  Serial.println("OpenLCD Example Code");

  mySerial.begin(9600); //Start communication with OpenLCD

  lcd.begin(mySerial);
  //Send contrast setting
//  mySerial.write('|'); //Put LCD into setting mode
//  mySerial.write(24); //Send contrast command
//  mySerial.write(contrast);
  lcd.setContrast(contrast);


}

void loop()
{
  //Send the clear command to the display - this returns the cursor to the beginning of the display
//  mySerial.write('|'); //Setting character
//  mySerial.write('-'); //Clear display
  lcd.clear();

  //mySerial.print("Hello World!    Counter: "); //For 16x2 LCD
  //mySerial.print("Hello World!        Counter: "); //For 20x4 LCD
  lcd.print("Hello World!        Counter: "); //For 20x4 LCD
  //mySerial.print(counter++);
  lcd.print(counter++);
  // mySerial.print(millis()/1000);
  delay(250); //Hang out for a bit
}
