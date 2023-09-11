#include "Display.h"

DisplayState displayState;

SerLCD display; // Initialize the library with default I2C address 0x72

void displayInit() {
  display.begin(Wire); //Set up the LCD for I2C communication

  display.setBacklight(255, 255, 255); //Set backlight to bright white
  display.setContrast(5); //Set contrast. Lower to 0 for higher contrast.

  display.clear(); //Clear the display - this moves the cursor to home position as well
}

String renderDoubleValue(double _value) {
  
  if (abs(_value) < 1) {
    _value = 0.0;
  }

  return String::format("%3.1f", _value);
}
