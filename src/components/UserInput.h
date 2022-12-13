#ifndef USER_INPUT_H
#define USER_INPUT_H

#include "Core.h"

#include <SparkFun_Qwiic_Button.h>

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
  // We need to evaluate the state of a button over time to determine
  // if it's a short press or a long press
  float buttonPressStartTimeMillis = -1;

  float lastUserInteractionTimeMillis = -1;
};

extern UserInputState userInputState;

void userInputInit();

void readUserInputState();

#endif