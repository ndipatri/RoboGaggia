/******************************************************************************
registers.h
Fischer Moseley @ SparkFun Electronics
Original Creation Date: July 24, 2019

This file defines the virtual memory map on the Qwiic Button/Switch. The enum
provides a set of pointers for the various registers on the Qwiic
Button/Switch, and the unions wrap the various bits in the bitfield in an easy
to use uint8_t format

Development environment specifics:
	IDE: Arduino 1.8.9
	Hardware Platform: Arduino Uno/SparkFun Redboard
	Qwiic Button Version: 1.0.0
    Qwiic Switch Version: 1.0.0

This code is Lemonadeware; if you see me (or any other SparkFun employee) at the
local, and you've found our code helpful, please buy us a round!

Distributed as-is; no warranty is given.
******************************************************************************/

//Register Pointer Map
enum Qwiic_Button_Register : uint8_t
{
    ID = 0x00,
    FIRMWARE_MINOR = 0x01,
    FIRMWARE_MAJOR = 0x02,
    BUTTON_STATUS = 0x03,
    INTERRUPT_CONFIG = 0x04,
    BUTTON_DEBOUNCE_TIME = 0x05,
    PRESSED_QUEUE_STATUS = 0x07,
    PRESSED_QUEUE_FRONT = 0x08,
    PRESSED_QUEUE_BACK = 0x0C,
    CLICKED_QUEUE_STATUS = 0x10,
    CLICKED_QUEUE_FRONT = 0x11,
    CLICKED_QUEUE_BACK = 0x15,
    LED_BRIGHTNESS = 0x19,
    LED_PULSE_GRANULARITY = 0x1A,
    LED_PULSE_CYCLE_TIME = 0x1B,
    LED_PULSE_OFF_TIME = 0x1D,
    I2C_ADDRESS = 0x1F,
};

typedef union {
    struct
    {
        bool eventAvailable : 1; //This is bit 0. User mutable, gets set to 1 when a new event occurs. User is expected to write 0 to clear the flag.
        bool hasBeenClicked : 1; //Defaults to zero on POR. Gets set to one when the button gets clicked. Must be cleared by the user.
        bool isPressed : 1;      //Gets set to one if button is pushed.
        bool : 5;
    };
    uint8_t byteWrapped;
} statusRegisterBitField;

typedef union {
    struct
    {
        bool clickedEnable : 1; //This is bit 0. user mutable, set to 1 to enable an interrupt when the button is clicked. Defaults to 0.
        bool pressedEnable : 1; //user mutable, set to 1 to enable an interrupt when the button is pressed. Defaults to 0.
        bool : 6;
    };
    uint8_t byteWrapped;
} interruptConfigBitField;

typedef union {
    struct
    {
        bool popRequest : 1; //This is bit 0. User mutable, user sets to 1 to pop from queue, we pop from queue and set the bit back to zero.
        bool isEmpty : 1;    //user immutable, returns 1 or 0 depending on whether or not the queue is empty
        bool isFull : 1;     //user immutable, returns 1 or 0 depending on whether or not the queue is full
        bool : 5;
    };
    uint8_t byteWrapped;
} queueStatusBitField;