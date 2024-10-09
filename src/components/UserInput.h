#ifndef USER_INPUT_H
#define USER_INPUT_H

#include "Common.h"
#include "Network.h"
#include "Bluetooth.h"

extern int RETURN_TO_HOME_INACTIVITY_MINUTES;

// Based on the physical button, we derive one of three
// input states
enum UserInputStateEnum {
  IDLE = 0,
  SHORT_PRESS = 1,
  LONG_PRESS = 2 
};
struct UserInputState {
  enum UserInputStateEnum state;

  float lastUserInteractionTimeMillis = -1;
};

extern UserInputState userInputState;

void readUserInputState();

#endif