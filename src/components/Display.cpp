#include "Display.h"

DisplayState displayState;

SerLCD display; // Initialize the library with default I2C address 0x72

void displayInit() {
  display.begin(Wire); //Set up the LCD for I2C communication

  display.setBacklight(255, 255, 255); //Set backlight to bright white
  display.setContrast(5); //Set contrast. Lower to 0 for higher contrast.

  display.clear(); //Clear the display - this moves the cursor to home position as well
}

String decodeMessageIfNecessary(char* _message, 
                                char* escapeSequence,
                                double firstValue,
                                double secondValue,                         
                                char* units,
                                String pattern) {
  char *message = _message; 

  if (strcmp(message, escapeSequence) == 0) {
    char firstValueString[9];
    sprintf(firstValueString, pattern, firstValue);

    char *decodedMessage;
    char lineBuff[20] = "                   ";
    if (secondValue != 0) {
      char secondValueString[9];
      sprintf(secondValueString, pattern, secondValue);

      decodedMessage = strcat(strcat(strcat(firstValueString, "/"), secondValueString), units);
    } else {
      decodedMessage = strcat(firstValueString, units);
    }

    return String((char *)(memcpy(lineBuff, decodedMessage, strlen(decodedMessage))));
  }

  return String("");
}

// If no decoding is necessary, the original string
// is returned
String decodeLongMessageIfNecessary(char* _message, 
                                    char* escapeSequence,
                                    long firstValue,
                                    long secondValue,                         
                                    char* units) {

    return decodeMessageIfNecessary(_message, escapeSequence, firstValue, secondValue, units, "%ld");
}

// If no decoding is necessary, the original string
// is returned
String decodeFloatMessageIfNecessary(char* _message, 
                                     char* escapeSequence,
                                     double firstValue,
                                     double secondValue,                         
                                     char* units) {
                          
    return decodeMessageIfNecessary(_message, escapeSequence, firstValue, secondValue, units, "%3.1f");
}
