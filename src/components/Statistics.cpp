#include "Statistics.h"

// TODO - Need to increase this after testing.
int MAX_BREW_COUNT_BEFORE_CLEANING = 10;

int BACKFLUSH_BREW_COUNT_EEPROM_ADDRESS = 0;
int TOTAL_BREW_COUNT_EEPROM_ADDRESS = 5;

int readBackflushBrewCount() {
  uint16_t value;
  EEPROM.get(BACKFLUSH_BREW_COUNT_EEPROM_ADDRESS, value);
  if(value == 0xFFFF) {
    // EEPROM was empty -> initialize value
    value = 0;
  }

  return value;
}

int readTotalBrewCount() {
  uint16_t value;
  EEPROM.get(TOTAL_BREW_COUNT_EEPROM_ADDRESS, value);
  if(value == 0xFFFF) {
    // EEPROM was empty -> initialize value
    value = 0;
  }

  return value;
}

boolean shouldBackflush() {
  int currentBrewCount = readBackflushBrewCount();  

  return (currentBrewCount > MAX_BREW_COUNT_BEFORE_CLEANING);
}

int shotsUntilBackflush() {
  return max(MAX_BREW_COUNT_BEFORE_CLEANING - readBackflushBrewCount(), 0);
}

void increaseBrewCount() {
  int backflushBrewCount = readBackflushBrewCount();  
  EEPROM.put(BACKFLUSH_BREW_COUNT_EEPROM_ADDRESS, backflushBrewCount + 1);

  int totalBrewCount = readTotalBrewCount();  
  EEPROM.put(TOTAL_BREW_COUNT_EEPROM_ADDRESS, totalBrewCount + 1);

}

void clearBackflushBrewCount() {
  EEPROM.put(BACKFLUSH_BREW_COUNT_EEPROM_ADDRESS, 0);
}



