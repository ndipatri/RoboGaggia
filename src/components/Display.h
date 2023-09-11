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

String renderDoubleValue(double _value);

String decodeLongMessageIfNecessary(char* _message, 
                                    char* escapeSequence,
                                    double firstValue,
                                    double secondValue,                         
                                    char* units);

String decodeFloatMessageIfNecessary(char* _message, 
                                     char* escapeSequence,
                                     double firstValue,
                                     double secondValue,                         
                                     char* units);


#endif                               