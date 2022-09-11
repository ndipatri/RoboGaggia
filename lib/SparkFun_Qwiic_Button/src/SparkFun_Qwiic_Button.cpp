/******************************************************************************
SparkFun_Qwiic_Button.cpp
SparkFun Qwiic Button/Switch Library Source File
Fischer Moseley @ SparkFun Electronics
Original Creation Date: July 24, 2019

This file implements the QwiicButton class, prototyped in SparkFun_Qwiic_Button.h

Development environment specifics:
	IDE: Arduino 1.8.9
	Hardware Platform: Arduino Uno/SparkFun Redboard
	Qwiic Button Version: 1.0.0
    Qwiic Switch Version: 1.0.0

This code is Lemonadeware; if you see me (or any other SparkFun employee) at the
local, and you've found our code helpful, please buy us a round!

Distributed as-is; no warranty is given.
******************************************************************************/

#include <Wire.h>
#include <SparkFun_Qwiic_Button.h>

#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif

/*-------------------------------- Device Status ------------------------*/

bool QwiicButton::begin(uint8_t address, TwoWire &wirePort)
{
    _deviceAddress = address; //grab the address that the sensor is on
    _i2cPort = &wirePort;     //grab the port that the user wants to use

    //return true if the device is connected and the device ID is what we expect
    return (isConnected() && checkDeviceID());
}

bool QwiicButton::isConnected()
{
    _i2cPort->beginTransmission(_deviceAddress);
    if (_i2cPort->endTransmission() == 0)
        return true;
    return false;
}

uint8_t QwiicButton::deviceID()
{
    return readSingleRegister(ID); //read and return the value in the ID register
}

bool QwiicButton::checkDeviceID()
{
    // return ((deviceID() == DEV_ID_SW) || (deviceID() == DEV_ID_BTN)); //Return true if the device ID matches either the button or the switch
    return (deviceID() == DEV_ID); //Return true if the device ID matches
}

uint8_t QwiicButton::getDeviceType()
{
    if (isConnected())
    { //only try to get the device ID if the device will acknowledge
        uint8_t id = deviceID();
        if (id == DEV_ID)
            return 1;
        // if (id == DEV_ID_SW)
        //     return 2;
    }
    return 0;
}

uint16_t QwiicButton::getFirmwareVersion()
{
    uint16_t version = (readSingleRegister(FIRMWARE_MAJOR)) << 8;
    version |= readSingleRegister(FIRMWARE_MINOR);
    return version;
}

bool QwiicButton::setI2Caddress(uint8_t address)
{
    if (address < 0x08 || address > 0x77)
    {
        Serial.println("Error1");
        return 1; //error immediately if the address is out of legal range
    }

    bool success = writeSingleRegister(I2C_ADDRESS, address);

    if (success == true)
    {
        _deviceAddress = address;
        return true;
    }

    else
    {
        Serial.println("Error2");
        return false;
    }
}

uint8_t QwiicButton::getI2Caddress()
{
    return _deviceAddress;
}

/*------------------------------ Button Status ---------------------- */
bool QwiicButton::isPressed()
{
    statusRegisterBitField statusRegister;
    statusRegister.byteWrapped = readSingleRegister(BUTTON_STATUS);
    return statusRegister.isPressed;
}

bool QwiicButton::hasBeenClicked()
{
    statusRegisterBitField statusRegister;
    statusRegister.byteWrapped = readSingleRegister(BUTTON_STATUS);
    return statusRegister.hasBeenClicked;
}

uint16_t QwiicButton::getDebounceTime()
{
    return readDoubleRegister(BUTTON_DEBOUNCE_TIME);
}

uint8_t QwiicButton::setDebounceTime(uint16_t time)
{
    return writeDoubleRegisterWithReadback(BUTTON_DEBOUNCE_TIME, time);
}

/*------------------- Interrupt Status/Configuration ---------------- */
uint8_t QwiicButton::enablePressedInterrupt()
{
    interruptConfigBitField interruptConfigure;
    interruptConfigure.byteWrapped = readSingleRegister(INTERRUPT_CONFIG);
    interruptConfigure.pressedEnable = 1;
    return writeSingleRegisterWithReadback(INTERRUPT_CONFIG, interruptConfigure.byteWrapped);
}

uint8_t QwiicButton::disablePressedInterrupt()
{
    interruptConfigBitField interruptConfigure;
    interruptConfigure.byteWrapped = readSingleRegister(INTERRUPT_CONFIG);
    interruptConfigure.pressedEnable = 0;
    return writeSingleRegisterWithReadback(INTERRUPT_CONFIG, interruptConfigure.byteWrapped);
}

uint8_t QwiicButton::enableClickedInterrupt()
{
    interruptConfigBitField interruptConfigure;
    interruptConfigure.byteWrapped = readSingleRegister(INTERRUPT_CONFIG);
    interruptConfigure.clickedEnable = 1;
    return writeSingleRegisterWithReadback(INTERRUPT_CONFIG, interruptConfigure.byteWrapped);
}

uint8_t QwiicButton::disableClickedInterrupt()
{
    interruptConfigBitField interruptConfigure;
    interruptConfigure.byteWrapped = readSingleRegister(INTERRUPT_CONFIG);
    interruptConfigure.clickedEnable = 0;
    return writeSingleRegisterWithReadback(INTERRUPT_CONFIG, interruptConfigure.byteWrapped);
}

bool QwiicButton::available()
{
    statusRegisterBitField buttonStatus;
    buttonStatus.byteWrapped = readSingleRegister(BUTTON_STATUS);
    return buttonStatus.eventAvailable;
}

uint8_t QwiicButton::clearEventBits()
{
    statusRegisterBitField buttonStatus;
    buttonStatus.byteWrapped = readSingleRegister(BUTTON_STATUS);
    buttonStatus.isPressed = 0;
    buttonStatus.hasBeenClicked = 0;
    buttonStatus.eventAvailable = 0;
    return writeSingleRegisterWithReadback(BUTTON_STATUS, buttonStatus.byteWrapped);
}

uint8_t QwiicButton::resetInterruptConfig()
{
    interruptConfigBitField interruptConfigure;
    interruptConfigure.pressedEnable = 1;
    interruptConfigure.clickedEnable = 1;
    return writeSingleRegisterWithReadback(INTERRUPT_CONFIG, interruptConfigure.byteWrapped);
    statusRegisterBitField buttonStatus;
    buttonStatus.eventAvailable = 0;
    return writeSingleRegisterWithReadback(BUTTON_STATUS, buttonStatus.byteWrapped);
}

/*------------------------- Queue Manipulation ---------------------- */
//pressed queue manipulation
bool QwiicButton::isPressedQueueFull()
{
    queueStatusBitField pressedQueueStatus;
    pressedQueueStatus.byteWrapped = readSingleRegister(PRESSED_QUEUE_STATUS);
    return pressedQueueStatus.isFull;
}

bool QwiicButton::isPressedQueueEmpty()
{
    queueStatusBitField pressedQueueStatus;
    pressedQueueStatus.byteWrapped = readSingleRegister(PRESSED_QUEUE_STATUS);
    return pressedQueueStatus.isEmpty;
}

unsigned long QwiicButton::timeSinceLastPress()
{
    return readQuadRegister(PRESSED_QUEUE_FRONT);
}

unsigned long QwiicButton::timeSinceFirstPress()
{
    return readQuadRegister(PRESSED_QUEUE_BACK);
}

unsigned long QwiicButton::popPressedQueue()
{
    unsigned long tempData = timeSinceFirstPress(); //grab the oldest value on the queue

    queueStatusBitField pressedQueueStatus;
    pressedQueueStatus.byteWrapped = readSingleRegister(PRESSED_QUEUE_STATUS);
    pressedQueueStatus.popRequest = 1;
    writeSingleRegister(PRESSED_QUEUE_STATUS, pressedQueueStatus.byteWrapped); //remove the oldest value from the queue

    return tempData; //return the value we popped
}

//clicked queue manipulation
bool QwiicButton::isClickedQueueFull()
{
    queueStatusBitField clickedQueueStatus;
    clickedQueueStatus.byteWrapped = readSingleRegister(CLICKED_QUEUE_STATUS);
    return clickedQueueStatus.isFull;
}

bool QwiicButton::isClickedQueueEmpty()
{
    queueStatusBitField clickedQueueStatus;
    clickedQueueStatus.byteWrapped = readSingleRegister(CLICKED_QUEUE_STATUS);
    return clickedQueueStatus.isEmpty;
}

unsigned long QwiicButton::timeSinceLastClick()
{
    return readQuadRegister(CLICKED_QUEUE_FRONT);
}

unsigned long QwiicButton::timeSinceFirstClick()
{
    return readQuadRegister(CLICKED_QUEUE_BACK);
}

unsigned long QwiicButton::popClickedQueue()
{
    unsigned long tempData = timeSinceFirstClick();
    queueStatusBitField clickedQueueStatus;
    clickedQueueStatus.byteWrapped = readSingleRegister(CLICKED_QUEUE_STATUS);
    clickedQueueStatus.popRequest = 1;
    writeSingleRegister(CLICKED_QUEUE_STATUS, clickedQueueStatus.byteWrapped);
    return tempData;
}

/*------------------------ LED Configuration ------------------------ */
bool QwiicButton::LEDconfig(uint8_t brightness, uint16_t cycleTime, uint16_t offTime, uint8_t granularity)
{
    bool success = writeSingleRegister(LED_BRIGHTNESS, brightness);
    success &= writeSingleRegister(LED_PULSE_GRANULARITY, granularity);
    success &= writeDoubleRegister(LED_PULSE_CYCLE_TIME, cycleTime);
    success &= writeDoubleRegister(LED_PULSE_OFF_TIME, offTime);
    return success;
}

bool QwiicButton::LEDoff()
{
    return LEDconfig(0, 0, 0);
}

bool QwiicButton::LEDon(uint8_t brightness)
{
    return LEDconfig(brightness, 0, 0);
}

/*------------------------- Internal I2C Abstraction ---------------- */

uint8_t QwiicButton::readSingleRegister(Qwiic_Button_Register reg)
{
    _i2cPort->beginTransmission(_deviceAddress);
    _i2cPort->write(reg);
    _i2cPort->endTransmission();

    //typecasting the 1 parameter in requestFrom so that the compiler
    //doesn't give us a warning about multiple candidates
    if (_i2cPort->requestFrom(_deviceAddress, static_cast<uint8_t>(1)) != 0)
    {
        return _i2cPort->read();
    }
    return 0;
}

uint16_t QwiicButton::readDoubleRegister(Qwiic_Button_Register reg)
{ //little endian
    _i2cPort->beginTransmission(_deviceAddress);
    _i2cPort->write(reg);
    _i2cPort->endTransmission();

    //typecasting the 2 parameter in requestFrom so that the compiler
    //doesn't give us a warning about multiple candidates
    if (_i2cPort->requestFrom(_deviceAddress, static_cast<uint8_t>(2)) != 0)
    {
        uint16_t data = _i2cPort->read();
        data |= (_i2cPort->read() << 8);
        return data;
    }
    return 0;
}

unsigned long QwiicButton::readQuadRegister(Qwiic_Button_Register reg)
{
    _i2cPort->beginTransmission(_deviceAddress);
    _i2cPort->write(reg);
    _i2cPort->endTransmission();

    union databuffer {
        uint8_t array[4];
        unsigned long integer;
    };

    databuffer data;

    //typecasting the 4 parameter in requestFrom so that the compiler
    //doesn't give us a warning about multiple candidates
    if (_i2cPort->requestFrom(_deviceAddress, static_cast<uint8_t>(4)) != 0)
    {
        for (uint8_t i = 0; i < 4; i++)
        {
            data.array[i] = _i2cPort->read();
        }
    }
    return data.integer;
}

bool QwiicButton::writeSingleRegister(Qwiic_Button_Register reg, uint8_t data)
{
    _i2cPort->beginTransmission(_deviceAddress);
    _i2cPort->write(reg);
    _i2cPort->write(data);
    if (_i2cPort->endTransmission() == 0)
        return true;
    return false;
}

bool QwiicButton::writeDoubleRegister(Qwiic_Button_Register reg, uint16_t data)
{
    _i2cPort->beginTransmission(_deviceAddress);
    _i2cPort->write(reg);
    _i2cPort->write(lowByte(data));
    _i2cPort->write(highByte(data));
    if (_i2cPort->endTransmission() == 0)
        return true;
    return false;
}

uint8_t QwiicButton::writeSingleRegisterWithReadback(Qwiic_Button_Register reg, uint8_t data)
{
    if (writeSingleRegister(reg, data))
        return 1;
    if (readSingleRegister(reg) != data)
        return 2;
    return 0;
}

uint16_t QwiicButton::writeDoubleRegisterWithReadback(Qwiic_Button_Register reg, uint16_t data)
{
    if (writeDoubleRegister(reg, data))
        return 1;
    if (readDoubleRegister(reg) != data)
        return 2;
    return 0;
}