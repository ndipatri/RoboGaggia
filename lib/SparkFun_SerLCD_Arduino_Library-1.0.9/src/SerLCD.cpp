/*
 * LCD library for SparkFun RGB 3.3v Serial Open LCD display
 * with an attached Qwiic adapter.
 *
 * By: Gaston R. Williams
 * Date: August 22, 2018
 * Update: March 23, 2020 - fixed missing return value in write(uint8_t)
 *
 * License: This code is public domain but you buy me a beer if you use this and we meet someday (Beerware license).
 *
 * This library is based heavily on the LiquidCrystal_I2C library and the sample code provided with
 * the SparkFun Serial OpenLCD display.  The original LiquidCrystal library by David A. Mellis and
 * modified by Limor Fried and the OpenLCD code by Nathan Seidle at SparkFun.
 *
 * The LiquidCrystal_I2C library was based on the work by DFRobot.
 * (That's the only attribution I found in the code I have. If anyone can provide better information,
 * Plese let me know and I'll be happy to give credit where credit is due.)
 *
 * Original information copied from OpenLCD:
 *
 * The OpenLCD display information is based based on code by
 * Nathan Seidle
 * SparkFun Electronics
 * Date: April 19th, 2015
 *
 * License: This code is public domain but you buy me a beer if you use this and we meet someday (Beerware license).
 * OpenLCD gives the user multiple interfaces (serial, I2C, and SPI) to control an LCD. SerLCD was the original
 * serial LCD from SparkFun that ran on the PIC 16F88 with only a serial interface and limited feature set.
 * This is an updated serial LCD.
 *
 * Please Note: 0x72 is the 7-bit I2C address. If you are using a different language than Arduino you will probably
 * need to add the Read/Write bit to the end of the address. This means the default read address for the OpenLCD
 * is 0b.1110.0101 or 0xE5 and the write address is 0b.1110.0100 or 0xE4.
 * For more information see https://learn.sparkfun.com/tutorials/i2c
 * Note: This code expects the display to be listening at the default I2C address. If your display is not at 0x72, you can
 * do a hardware reset. Tie the RX pin to ground and power up OpenLCD. You should see the splash screen
 * then "System reset Power cycle me" and the backlight will begin to blink. Now power down OpenLCD and remove
 * the RX/GND jumper. OpenLCD is now reset.
 *
 * To get this code to work, attach a Qwiic adapter to an OpenLCD. Use the Qwiic cable to attach adapter to a SparkFun Blackboard or
 * an Arduino Uno with the Qwiic shield.
 *
 * The OpenLCD has 4.7k pull up resistors on SDA and SCL. If you have other devices on the
 * I2C bus then you may want to disable the pull up resistors by clearing the PU (pull up) jumper.

 * OpenLCD will work at 400kHz Fast I2C. Use the .setClock() call shown below to set the data rate
 * faster if needed.
 * Command cheat sheet:
 * ASCII / DEC / HEX
 * '|'    / 124 / 0x7C - Put into setting mode
 * Ctrl+c / 3 / 0x03 - Change width to 20
 * Ctrl+d / 4 / 0x04 - Change width to 16
 * Ctrl+e / 5 / 0x05 - Change lines to 4
 * Ctrl+f / 6 / 0x06 - Change lines to 2
 * Ctrl+g / 7 / 0x07 - Change lines to 1
 * Ctrl+h / 8 / 0x08 - Software reset of the system
 * Ctrl+i / 9 / 0x09 - Enable/disable splash screen
 * Ctrl+j / 10 / 0x0A - Save currently displayed text as splash
 * Ctrl+k / 11 / 0x0B - Change baud to 2400bps
 * Ctrl+l / 12 / 0x0C - Change baud to 4800bps
 * Ctrl+m / 13 / 0x0D - Change baud to 9600bps
 * Ctrl+n / 14 / 0x0E - Change baud to 14400bps
 * Ctrl+o / 15 / 0x0F - Change baud to 19200bps
 * Ctrl+p / 16 / 0x10 - Change baud to 38400bps
 * Ctrl+q / 17 / 0x11 - Change baud to 57600bps
 * Ctrl+r / 18 / 0x12 - Change baud to 115200bps
 * Ctrl+s / 19 / 0x13 - Change baud to 230400bps
 * Ctrl+t / 20 / 0x14 - Change baud to 460800bps
 * Ctrl+u / 21 / 0x15 - Change baud to 921600bps
 * Ctrl+v / 22 / 0x16 - Change baud to 1000000bps
 * Ctrl+w / 23 / 0x17 - Change baud to 1200bps
 * Ctrl+x / 24 / 0x18 - Change the contrast. Follow Ctrl+x with number 0 to 255. 120 is default.
 * Ctrl+y / 25 / 0x19 - Change the TWI address. Follow Ctrl+x with number 0 to 255. 114 (0x72) is default.
 * Ctrl+z / 26 / 0x1A - Enable/disable ignore RX pin on startup (ignore emergency reset)
 * '+'    / 43 / 0x2B - Set RGB backlight with three following bytes, 0-255
 * ','    / 44 / 0x2C - Display current firmware version
 * '-'    / 45 / 0x2D - Clear display. Move cursor to home position.
 * '.'    / 46 / 0x2E - Enable system messages (ie, display 'Contrast: 5' when changed)
 * '/'    / 47 / 0x2F - Disable system messages (ie, don't display 'Contrast: 5' when changed)
 * '0'    / 48 / 0x30 - Enable splash screen
 * '1'    / 49 / 0x31 - Disable splash screen
 *        / 128-157 / 0x80-0x9D - Set the primary backlight brightness. 128 = Off, 157 = 100%.
 *        / 158-187 / 0x9E-0xBB - Set the green backlight brightness. 158 = Off, 187 = 100%.
 *        / 188-217 / 0xBC-0xD9 - Set the blue backlight brightness. 188 = Off, 217 = 100%.
 *		For example, to change the baud rate to 115200 send 124 followed by 18.
 *
 */
#include "SerLCD.h"

//<<constructor>> setup using defaults
SerLCD::SerLCD()
{
}

//<<destructor>>
SerLCD::~SerLCD()
{ /*nothing to destruct*/
}

/*
 * Set up the i2c communication with the SerLCD.
 * wirePort - TwoWire port
 * ic2_addr - I2C address
 */
void SerLCD::begin(TwoWire &wirePort, byte i2c_addr)
{
  _i2cAddr = i2c_addr;

  begin(wirePort);
} // begin

/*
 * Set up the i2c communication with the SerLCD.
 */
void SerLCD::begin(TwoWire &wirePort)
{
  _i2cPort = &wirePort; //Grab which port the user wants us to use
  _serialPort = NULL;   //Set to null to be safe
  _spiPort = NULL;      //Set to null to be safe

  //Call init function since display may have been left in unknown state
  init();
} // begin

/*
 * Set up the serial communication with the SerLCD.
 */
void SerLCD::begin(Stream &serialPort)
{
  _serialPort = &serialPort; //Grab which port the user wants us to use
  _i2cPort = NULL;           //Set to null to be safe
  _spiPort = NULL;           //Set to null to be safe

  //Call init function since display may have been left in unknown state
  init();
} // begin

//Only available in Arduino 1.6 or later
#ifdef SPI_HAS_TRANSACTION
/*
 * Set up the SPI communication with the SerLCD using SPI transactions
 *
 * NB we pass SPISettings by value, since many of the examples for the SPI
 * transactions create the settings object in the function call, and that only
 * works if the function passes the object by value.
 */
void SerLCD::begin(SPIClass &spiPort, byte csPin, SPISettings spiSettings)
{
  _spiSettings = spiSettings;
  _spiTransaction = true;

  begin(spiPort, csPin);
} // begin
#endif

/*
 * Set up the SPI communication with the SerLCD.
 */
void SerLCD::begin(SPIClass &spiPort, byte csPin)
{
  _csPin = csPin;

  pinMode(csPin, OUTPUT);    //set pin to output, in case user forgot
  digitalWrite(csPin, HIGH); //deselect dispaly, in case user forgot

  _spiPort = &spiPort; //Grab the port the user wants us to use
  _i2cPort = NULL;     //Set to null to be safe
  _serialPort = NULL;  //Set to null to be safe

  _spiPort->begin(); //call begin, in case the user forgot

  //Call init function since display may have been left in unknown state
  init();
} // begin

//private functions for serial transmission
/*
 * Begin transmission to the device
 */
void SerLCD::beginTransmission()
{
  //do nothing if using serialPort
  if (_i2cPort)
  {
    _i2cPort->beginTransmission(_i2cAddr); // transmit to device
  }
  else if (_spiPort)
  {
#ifdef SPI_HAS_TRANSACTION
    if (_spiTransaction)
    {
      _spiPort->beginTransaction(_spiSettings); //gain control of the SPI bus
    }                                           //if _spiSettings
#endif
    digitalWrite(_csPin, LOW);
    delay(10); //wait a bit for display to enable
  }            // if-else
} //beginTransmission

/*
 * Send data to the device
 *
 * data - byte to send
 */
void SerLCD::transmit(uint8_t data)
{
  if (_i2cPort)
  {
    _i2cPort->write(data); // transmit to device
  }
  else if (_serialPort)
  {
    _serialPort->write(data);
  }
  else if (_spiPort)
  {
    _spiPort->transfer(data);
  } // if-else
} //transmit

/*
 * Begin transmission to the device
 */
void SerLCD::endTransmission()
{
  //do nothing if using Serial port
  if (_i2cPort)
  {
    _i2cPort->endTransmission(); // transmit to device
  }
  else if (_spiPort)
  {
    digitalWrite(_csPin, HIGH); //disable display
#ifdef SPI_HAS_TRANSACTION
    if (_spiTransaction)
    {
      _spiPort->endTransaction(); //let go of the SPI bus
    }                             //if _spiSettings
#endif
    delay(10); //wait a bit for display to disable
  }            // if-else
} //beginTransmission

/*
 * Initialize the display
 *
 */
void SerLCD::init()
{
  beginTransmission();
  transmit(SPECIAL_COMMAND);                      //Send special command character
  transmit(LCD_DISPLAYCONTROL | _displayControl); //Send the display command
  transmit(SPECIAL_COMMAND);                      //Send special command character
  transmit(LCD_ENTRYMODESET | _displayMode);      //Send the entry mode command
  transmit(SETTING_COMMAND);                      //Put LCD into setting mode
  transmit(CLEAR_COMMAND);                        //Send clear display command
  endTransmission();                              //Stop transmission
  delay(50);                                      //let things settle a bit
} //init

/*
  * Send a command to the display.
  * Used by other functions.
  *
  * byte command to send
  */
void SerLCD::command(byte command)
{
  beginTransmission();       // transmit to device
  transmit(SETTING_COMMAND); //Put LCD into setting mode
  transmit(command);         //Send the command code
  endTransmission();         //Stop transmission

  delay(10); //Hang out for a bit
}

/*
 * Send a special command to the display.  Used by other functions.
 *
 * byte command to send
 */
void SerLCD::specialCommand(byte command)
{
  beginTransmission();       // transmit to device
  transmit(SPECIAL_COMMAND); //Send special command character
  transmit(command);         //Send the command code
  endTransmission();         //Stop transmission

  delay(50); //Wait a bit longer for special display commands
}

/*
 * Send multiple special commands to the display.
 * Used by other functions.
 *
 * byte command to send
 * byte count number of times to send
 */
void SerLCD::specialCommand(byte command, byte count)
{
  beginTransmission(); // transmit to device

  for (int i = 0; i < count; i++)
  {
    transmit(SPECIAL_COMMAND); //Send special command character
    transmit(command);         //Send command code
  }                            // for
  endTransmission();           //Stop transmission

  delay(50); //Wait a bit longer for special display commands
}

/*
 * Send the clear command to the display.  This clears the
 * display and forces the cursor to return to the beginning
 * of the display.
 */
void SerLCD::clear()
{
  command(CLEAR_COMMAND);
  delay(10); // a little extra delay after clear
}

/*
 * Send the home command to the display.  This returns the cursor
 * to return to the beginning of the display, without clearing
 * the display.
 */
void SerLCD::home()
{
  specialCommand(LCD_RETURNHOME);
}

/*
 * Set the cursor position to a particular column and row.
 *
 * column - byte 0 to 19
 * row - byte 0 to 3
 *
 */
void SerLCD::setCursor(byte col, byte row)
{
  int row_offsets[] = {0x00, 0x40, 0x14, 0x54};

  //kepp variables in bounds
  //Explicitly cast numeric literals to type byte to avoid ESP32 and ESP8266 compile errors
  row = max((byte) 0, row);            //row cannot be less than 0
  row = min(row, (byte)(MAX_ROWS - 1)); //row cannot be greater than max rows

  //send the command
  specialCommand(LCD_SETDDRAMADDR | (col + row_offsets[row]));
} // setCursor

/*
 * Create a customer character
 * byte   location - character number 0 to 7
 * byte[] charmap  - byte array for character
 */
void SerLCD::createChar(byte location, byte charmap[])
{
  location &= 0x7; // we only have 8 locations 0-7
  beginTransmission();
  //Send request to create a customer character
  transmit(SETTING_COMMAND); //Put LCD into setting mode
  transmit(27 + location);
  for (int i = 0; i < 8; i++)
  {
    transmit(charmap[i]);
  } // for
  endTransmission();
  delay(50); //This takes a bit longer
}

/*
 * Write a customer character to the display
 *
 * byte location - character number 0 to 7
 */
void SerLCD::writeChar(byte location)
{
  location &= 0x7; // we only have 8 locations 0-7

  command(35 + location);
}

/*
 * Write a byte to the display.
 * Required for Print.
 */
size_t SerLCD::write(uint8_t b)
{
  beginTransmission(); // transmit to device
  transmit(b);
  endTransmission(); //Stop transmission
  delay(10);         // wait a bit
  return 1;
} // write

/*
 * Write a character buffer to the display.
 * Required for Print.
 */
size_t SerLCD::write(const uint8_t *buffer, size_t size)
{
  size_t n = 0;
  beginTransmission(); // transmit to device
  while (size--)
  {
    transmit(*buffer++);
    n++;
  }                  //while
  endTransmission(); //Stop transmission
  delay(10);         //
  return n;
} //write

/*
 * Write a string to the display.
 * Required for Print.
 */
size_t SerLCD::write(const char *str)
{
  if (str == NULL)
    return 0;
  return write((const uint8_t *)str, strlen(str));
}

/*
  * Turn the display off quickly.
  */
void SerLCD::noDisplay()
{
  _displayControl &= ~LCD_DISPLAYON;
  specialCommand(LCD_DISPLAYCONTROL | _displayControl);
} // noDisplay

/*
 * Turn the display on quickly.
 */
void SerLCD::display()
{
  _displayControl |= LCD_DISPLAYON;
  specialCommand(LCD_DISPLAYCONTROL | _displayControl);
} // display

/*
  * Turn the underline cursor off.
  */
void SerLCD::noCursor()
{
  _displayControl &= ~LCD_CURSORON;
  specialCommand(LCD_DISPLAYCONTROL | _displayControl);
} // noCursor

/*
 * Turn the underline cursor on.
 */
void SerLCD::cursor()
{
  _displayControl |= LCD_CURSORON;
  specialCommand(LCD_DISPLAYCONTROL | _displayControl);
} // cursor

/*
  * Turn the blink cursor off.
  */
void SerLCD::noBlink()
{
  _displayControl &= ~LCD_BLINKON;
  specialCommand(LCD_DISPLAYCONTROL | _displayControl);
} // noBlink

/*
 * Turn the blink cursor on.
 */
void SerLCD::blink()
{
  _displayControl |= LCD_BLINKON;
  specialCommand(LCD_DISPLAYCONTROL | _displayControl);
} // blink

/*
 * Scroll the display one character to the left, without
 * changing the text
 */
void SerLCD::scrollDisplayLeft()
{
  specialCommand(LCD_CURSORSHIFT | LCD_DISPLAYMOVE | LCD_MOVELEFT);
} // scrollDisplayLeft

/*
 * Scroll the display multiple characters to the left, without
 * changing the text
 *
 * count byte - number of characters to scroll
 */
void SerLCD::scrollDisplayLeft(byte count)
{
  specialCommand(LCD_CURSORSHIFT | LCD_DISPLAYMOVE | LCD_MOVELEFT, count);
} // scrollDisplayLeft

/*
 * Scroll the display one character to the right, without
 * changing the text
 */
void SerLCD::scrollDisplayRight()
{
  specialCommand(LCD_CURSORSHIFT | LCD_DISPLAYMOVE | LCD_MOVERIGHT);
} // scrollDisplayRight

/*
 * Scroll the display multiple characters to the right, without
 * changing the text
 *
 * count byte - number of characters to scroll
 */
void SerLCD::scrollDisplayRight(byte count)
{
  specialCommand(LCD_CURSORSHIFT | LCD_DISPLAYMOVE | LCD_MOVERIGHT, count);
} // scrollDisplayRight

/*
 *  Move the cursor one character to the left.
 */
void SerLCD::moveCursorLeft()
{
  specialCommand(LCD_CURSORSHIFT | LCD_CURSORMOVE | LCD_MOVELEFT);
} // moveCursorLeft

/*
 *  Move the cursor multiple characters to the left.
 *
 *  count byte - number of characters to move
 */
void SerLCD::moveCursorLeft(byte count)
{
  specialCommand(LCD_CURSORSHIFT | LCD_CURSORMOVE | LCD_MOVELEFT, count);
} // moveCursorLeft

/*
 *  Move the cursor one character to the right.
 */
void SerLCD::moveCursorRight()
{
  specialCommand(LCD_CURSORSHIFT | LCD_CURSORMOVE | LCD_MOVERIGHT);
} // moveCursorRight

/*
 *  Move the cursor multiple characters to the right.
 *
 *  count byte - number of characters to move
 */
void SerLCD::moveCursorRight(byte count)
{
  specialCommand(LCD_CURSORSHIFT | LCD_CURSORMOVE | LCD_MOVERIGHT, count);
} // moveCursorRight

/*
 * Use a standard hex rgb value (0x00000000 to 0x00FFFFFF) to set
 * the backlight color.
 *
 * The encoded long value has form (0x00RRGGBB) where RR, GG and BB
 * are red, green, blue byte values in hex.  The remaining two most
 * significant bytes of the long value are ignored.
 *
 * rgb - unsigned long hex encoded rgb value.
 */
void SerLCD::setBacklight(unsigned long rgb)
{
  // convert from hex triplet to byte values
  byte r = (rgb >> 16) & 0x0000FF;
  byte g = (rgb >> 8) & 0x0000FF;
  byte b = rgb & 0x0000FF;

  setBacklight(r, g, b);
}

/*
 * Uses a standard rgb byte triplit eg. (255, 0, 255) to
 * set the backlight color.
 */
void SerLCD::setBacklight(byte r, byte g, byte b)
{
  // map the byte value range to backlight command range
  byte red = 128 + map(r, 0, 255, 0, 29);
  byte green = 158 + map(g, 0, 255, 0, 29);
  byte blue = 188 + map(b, 0, 255, 0, 29);

  //send commands to the display to set backlights
  beginTransmission(); // transmit to device

  //Turn display off to hide confirmation messages
  _displayControl &= ~LCD_DISPLAYON;
  transmit(SPECIAL_COMMAND); //Send special command character
  transmit(LCD_DISPLAYCONTROL | _displayControl);

  //Set the red, green and blue values
  transmit(SETTING_COMMAND); //Set red backlight amount
  transmit(red);
  transmit(SETTING_COMMAND); //Set green backlight amount
  transmit(green);
  transmit(SETTING_COMMAND); //Set blue backlight amount
  transmit(blue);

  //Turn display back on and end
  _displayControl |= LCD_DISPLAYON;
  transmit(SPECIAL_COMMAND);                      //Send special command character
  transmit(LCD_DISPLAYCONTROL | _displayControl); //Turn display on as before
  endTransmission();                              //Stop transmission
  delay(50);                                      //This one is a bit slow
} // setBacklight

// New backlight function
void SerLCD::setFastBacklight(unsigned long rgb)
{
  // convert from hex triplet to byte values
  byte r = (rgb >> 16) & 0x0000FF;
  byte g = (rgb >> 8) & 0x0000FF;
  byte b = rgb & 0x0000FF;

  setFastBacklight(r, g, b);
}

//New command - set backlight with LCD messages or delays
void SerLCD::setFastBacklight(byte r, byte g, byte b)
{
  //send commands to the display to set backlights
  beginTransmission();       // transmit to device
  transmit(SETTING_COMMAND); //Send special command character
  transmit(SET_RGB_COMMAND); //Send the set RGB character '+' or plus
  transmit(r);               //Send the red value
  transmit(g);               //Send the green value
  transmit(b);               //Send the blue value
  endTransmission();         //Stop transmission
  delay(10);
} // setFastBacklight

//Enable system messages
//This allows user to see printing messages like 'UART: 57600' and 'Contrast: 5'
void SerLCD::enableSystemMessages()
{
  beginTransmission();                     // transmit to device
  transmit(SETTING_COMMAND);               //Send special command character
  transmit(ENABLE_SYSTEM_MESSAGE_DISPLAY); //Send the set '.' character
  endTransmission();                       //Stop transmission
  delay(10);
}

//Disable system messages
//This allows user to disable printing messages like 'UART: 57600' and 'Contrast: 5'
void SerLCD::disableSystemMessages()
{
  beginTransmission();                      // transmit to device
  transmit(SETTING_COMMAND);                //Send special command character
  transmit(DISABLE_SYSTEM_MESSAGE_DISPLAY); //Send the set '.' character
  endTransmission();                        //Stop transmission
  delay(10);
}

//Enable splash screen at power on
void SerLCD::enableSplash()
{
  beginTransmission();             // transmit to device
  transmit(SETTING_COMMAND);       //Send special command character
  transmit(ENABLE_SPLASH_DISPLAY); //Send the set '.' character
  endTransmission();               //Stop transmission
  delay(10);
}

//Disable splash screen at power on
void SerLCD::disableSplash()
{
  beginTransmission();              // transmit to device
  transmit(SETTING_COMMAND);        //Send special command character
  transmit(DISABLE_SPLASH_DISPLAY); //Send the set '.' character
  endTransmission();                //Stop transmission
  delay(10);
}

//Save the current display as the splash
void SerLCD::saveSplash()
{
  //Save whatever is currently being displayed into EEPROM
  //This will be displayed at next power on as the splash screen
  beginTransmission();                      // transmit to device
  transmit(SETTING_COMMAND);                //Send special command character
  transmit(SAVE_CURRENT_DISPLAY_AS_SPLASH); //Send the set Ctrl+j character
  endTransmission();                        //Stop transmission
  delay(10);
}

/*
 * Set the text to flow from left to right.  This is the direction
 * that is common to most Western languages.
 */
void SerLCD::leftToRight()
{
  _displayMode |= LCD_ENTRYLEFT;
  specialCommand(LCD_ENTRYMODESET | _displayMode);
} // leftToRight

/*
 * Set the text to flow from right to left.
 */
void SerLCD::rightToLeft()
{
  _displayMode &= ~LCD_ENTRYLEFT;
  specialCommand(LCD_ENTRYMODESET | _displayMode);
} //rightToLeft

/*
 * Turn autoscrolling on. This will 'right justify' text from
 * the cursor.
 */
void SerLCD::autoscroll()
{
  _displayMode |= LCD_ENTRYSHIFTINCREMENT;
  specialCommand(LCD_ENTRYMODESET | _displayMode);
} //autoscroll

/*
 * Turn autoscrolling off.
 */
void SerLCD::noAutoscroll()
{
  _displayMode &= ~LCD_ENTRYSHIFTINCREMENT;
  specialCommand(LCD_ENTRYMODESET | _displayMode);
} //noAutoscroll

/*
 * Change the contrast from 0 to 255. 120 is default.
 *
 * byte new_val - new contrast value
 */
void SerLCD::setContrast(byte new_val)
{
  //send commands to the display to set backlights
  beginTransmission();        // transmit to device
  transmit(SETTING_COMMAND);  //Send contrast command
  transmit(CONTRAST_COMMAND); //0x18
  transmit(new_val);          //Send new contrast value
  endTransmission();          //Stop transmission

  delay(10); //Wait a little bit
} //setContrast

/*
 * Change the I2C Address. 0x72 is the default.
 * Note that this change is persistent.  If anything
 * goes wrong you may need to do a hardware reset
 * to unbrick the display.
 *
 * byte new_addr - new i2c address
 */
void SerLCD::setAddress(byte new_addr)
{
  //send commands to the display to set backlights
  beginTransmission();       // transmit to device on old address
  transmit(SETTING_COMMAND); //Send contrast command
  transmit(ADDRESS_COMMAND); //0x19
  transmit(new_addr);        //Send new contrast value
  endTransmission();         //Stop transmission

  //Update our own address so we can still talk to the display
  _i2cAddr = new_addr;

  delay(50); //This may take awhile
} //setAddress

/*
 * getAddress
 * 
 * Returns private variable I2C address
 */
byte SerLCD::getAddress()
{
  return _i2cAddr;
} //getAddress