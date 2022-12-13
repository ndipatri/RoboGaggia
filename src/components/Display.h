#ifndef DISPLAY_H
#define DISPLAY_H

#include <SerLCD.h>

struct DisplayState {
  String display1;
  String display2;
  String display3;
  String display4;
};

extern DisplayState displayState;

extern SerLCD display; // Initialize the library with default I2C address 0x72

void displayInit();

String decodeMessageIfNecessary(char* _message, 
                                char* escapeSequence,
                                long firstValue,
                                long secondValue,                         
                                char* units);

#endif                               