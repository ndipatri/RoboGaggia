/******************************************************************************
SparkFun_Qwiic_Button.h
SparkFun Qwiic Button/Switch Library Header File
Fischer Moseley @ SparkFun Electronics
Original Creation Date: July 24, 2019
https://github.com/sparkfunX/

This file prototypes the QwiicButton class, implemented in SparkFun_Qwiic_Button.cpp.

Development environment specifics:
	IDE: Arduino 1.8.9
	Hardware Platform: Arduino Uno/SparkFun Redboard
	Qwiic Button Breakout Version: 1.0.0
    Qwiic Switch Breakout Version: 1.0.0

This code is Lemonadeware; if you see me (or any other SparkFun employee) at the
local, and you've found our code helpful, please buy us a round!

Distributed as-is; no warranty is given.
******************************************************************************/

#ifndef __SparkFun_Qwiic_Button_H__
#define __SparkFun_Qwiic_Button_H__

#include <Wire.h>
#include <Arduino.h>
#include "registers.h"

#define DEFAULT_ADDRESS 0x6F //default I2C address of the button
#define DEV_ID 0x5D          //device ID of the Qwiic Button

class QwiicButton
{
private:
    TwoWire *_i2cPort;      //Generic connection to user's chosen I2C port
    uint8_t _deviceAddress; //I2C address of the button/switch

public:
    //Device status
    bool begin(uint8_t address = DEFAULT_ADDRESS, TwoWire &wirePort = Wire); //Sets device I2C address to a user-specified address, over whatever port the user specifies.
    bool isConnected();                                                      //Returns true if the button/switch will acknowledge over I2C, and false otherwise
    uint8_t deviceID();                                                      //Return the 8-bit device ID of the attached device.
    bool checkDeviceID();                                                    //Returns true if the device ID matches that of either the button or the switch
    uint8_t getDeviceType();                                                 //Returns 1 if a button is attached, 2 if a switch is attached. Returns 0 if there is no device attached.
    uint16_t getFirmwareVersion();                                           //Returns the firmware version of the attached device as a 16-bit integer. The leftmost (high) byte is the major revision number, and the rightmost (low) byte is the minor revision number.
    bool setI2Caddress(uint8_t address);                                     //Configures the attached device to attach to the I2C bus using the specified address
    uint8_t getI2Caddress();                                                 //Returns the I2C address of the device.

    //Button status/config
    bool isPressed();                       //Returns 1 if the button/switch is pressed, and 0 otherwise
    bool hasBeenClicked();                  //Returns 1 if the button/switch is clicked, and 0 otherwise
    uint16_t getDebounceTime();             //Returns the time that the button waits for the mechanical contacts to settle, (in milliseconds).
    uint8_t setDebounceTime(uint16_t time); //Sets the time that the button waits for the mechanical contacts to settle (in milliseconds) and checks if the register was set properly. Returns 0 on success, 1 on register I2C write fail, and 2 if the value didn't get written into the register properly.

    //Interrupt status/config
    uint8_t enablePressedInterrupt();  //When called, the interrupt will be configured to trigger when the button is pressed. If enableClickedInterrupt() has also been called, then the interrupt will trigger on either a push or a click.
    uint8_t disablePressedInterrupt(); //When called, the interrupt will no longer be configured to trigger when the button is pressed. If enableClickedInterrupt() has also been called, then the interrupt will still trigger on the button click.
    uint8_t enableClickedInterrupt();  //When called, the interrupt will be configured to trigger when the button is clicked. If enablePressedInterrupt() has also been called, then the interrupt will trigger on either a push or a click.
    uint8_t disableClickedInterrupt(); //When called, the interrupt will no longer be configured to trigger when the button is clicked. If enablePressedInterrupt() has also been called, then the interrupt will still trigger on the button press.
    bool available();                  //Returns the eventAvailable bit
    uint8_t clearEventBits();          //Sets isPressed, hasBeenClicked, and eventAvailable to zero
    uint8_t resetInterruptConfig();    //Resets the interrupt configuration back to defaults.

    //Queue manipulation
    bool isPressedQueueFull();           //Returns true if the queue of button press timestamps is full, and false otherwise.
    bool isPressedQueueEmpty();          //Returns true if the queue of button press timestamps is empty, and false otherwise.
    unsigned long timeSinceLastPress();  //Returns how many milliseconds it has been since the last button press. Since this returns a 32-bit unsigned int, it will roll over about every 50 days.
    unsigned long timeSinceFirstPress(); //Returns how many milliseconds it has been since the first button press. Since this returns a 32-bit unsigned int, it will roll over about every 50 days.
    unsigned long popPressedQueue();     //Returns the oldest value in the queue (milliseconds since first button press), and then removes it.

    bool isClickedQueueFull();           //Returns true if the queue of button click timestamps is full, and false otherwise.
    bool isClickedQueueEmpty();          //Returns true if the queue of button press timestamps is empty, and false otherwise.
    unsigned long timeSinceLastClick();  //Returns how many milliseconds it has been since the last button click. Since this returns a 32-bit unsigned int, it will roll over about every 50 days.
    unsigned long timeSinceFirstClick(); //Returns how many milliseconds it has been since the first button click. Since this returns a 32-bit unsigned int, it will roll over about every 50 days.
    unsigned long popClickedQueue();     //Returns the oldest value in the queue (milliseconds since first button click), and then removes it.

    //LED configuration
    bool LEDconfig(uint8_t brightness, uint16_t cycleTime,
                   uint16_t offTime, uint8_t granularity = 1); //Configures the LED with the given max brightness, granularity (1 is fine for most applications), cycle time, and off time.
    bool LEDoff();                                             //Turns the onboard LED off
    bool LEDon(uint8_t brightness = 255);                      //Turns the onboard LED on with specified brightness. Set brightness to an integer between 0 and 255, where 0 is off and 255 is max brightness.

    //Internal I2C Abstraction
    uint8_t readSingleRegister(Qwiic_Button_Register reg);                              //Reads a single 8-bit register.
    uint16_t readDoubleRegister(Qwiic_Button_Register reg);                             //Reads a 16-bit register (little endian).
    unsigned long readQuadRegister(Qwiic_Button_Register reg);                          //Reads a 32-bit register (little endian).
    bool writeSingleRegister(Qwiic_Button_Register reg, uint8_t data);                  //Attempts to write data into a single 8-bit register. Does not check to make sure it was written successfully. Returns 0 if there wasn't an error on I2C transmission, and 1 otherwise.
    bool writeDoubleRegister(Qwiic_Button_Register reg, uint16_t data);                 //Attempts to write data into a double (two 8-bit) registers. Does not check to make sure it was written successfully. Returns 0 if there wasn't an error on I2C transmission, and 1 otherwise.
    uint8_t writeSingleRegisterWithReadback(Qwiic_Button_Register reg, uint8_t data);   //Writes data into a single 8-bit register, and checks to make sure that the data was written successfully. Returns 0 on no error, 1 on I2C write fail, and 2 if the register doesn't read back the same value that was written.
    uint16_t writeDoubleRegisterWithReadback(Qwiic_Button_Register reg, uint16_t data); //Writes data into a double (two 8-bit) registers, and checks to make sure that the data was written successfully. Returns 0 on no error, 1 on I2C write fail, and 2 if the register doesn't read back the same value that was written.
};
#endif