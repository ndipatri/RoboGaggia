#include "Cleaning.h"

// TODO - Need to increase this after testing.
int MAX_BREW_COUNT_BEFORE_CLEANING = 2;

int BREW_COUNT_EEPROM_ADDRESS = 0;

boolean shouldBackflush() {
  int currentBrewCount = readBrewCount();  

  return (currentBrewCount > MAX_BREW_COUNT_BEFORE_CLEANING);
}

void increaseBrewCountForBackflush() {
  int currentBrewCount = readBrewCount();  

  EEPROM.put(BREW_COUNT_EEPROM_ADDRESS, currentBrewCount + 1);
}

void clearBrewCountForBackflush() {
  EEPROM.put(BREW_COUNT_EEPROM_ADDRESS, 0);
}

int readBrewCount() {
  uint16_t value;
  EEPROM.get(BREW_COUNT_EEPROM_ADDRESS, value);
  if(value == 0xFFFF) {
    // EEPROM was empty -> initialize value
    value = 0;
  }

  return value;
}


