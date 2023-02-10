
#include "UserInput.h"


// Based on the physical button, we derive one of three
// input states
UserInputState userInputState;

QwiicButton userButton;

// How long the button has to be pressed before it is considered
// a long press
float BUTTON_LONG_PRESS_DURATION_MILLIS = 1000;

// This can be considered an 'auto-off' feature as the HOME state represents
// effectively NO BEHAVIOR (e.g. heater is off, no extraction, no telemetry being sent)
int RETURN_TO_HOME_INACTIVITY_MINUTES = 30;

void readUserInputState() {
  // Derive current user input state

  float nowTimeMillis = millis();

  if (userButton.isPressed()) {
    if (userInputState.buttonPressStartTimeMillis < 0) {  
      // rising edge detected...
      userInputState.buttonPressStartTimeMillis = nowTimeMillis;
    }
  
    // if long press hasn't already been identified (parking on button), and we identify a LONG press...
    if ((userInputState.buttonPressStartTimeMillis != 0) && 
        (nowTimeMillis - userInputState.buttonPressStartTimeMillis > BUTTON_LONG_PRESS_DURATION_MILLIS)) {
      userInputState.state = LONG_PRESS;
      userInputState.buttonPressStartTimeMillis = 0; // this indicates a LONG press was already identified..
      userInputState.lastUserInteractionTimeMillis = nowTimeMillis;

      return;
    }
  } else {
    if (userInputState.buttonPressStartTimeMillis > 0) {  
      // falling edge detected...
      if (nowTimeMillis - userInputState.buttonPressStartTimeMillis < BUTTON_LONG_PRESS_DURATION_MILLIS) {
        userInputState.state = SHORT_PRESS;
        userInputState.buttonPressStartTimeMillis = -1;
        userInputState.lastUserInteractionTimeMillis = nowTimeMillis;

        return;
      }
    } else {
      // button is not being pressed
      
      // we do this because we are in a 'button idle' state and we want to ensure this 
      // is reset.. it could have been @ -999 due to a extended LONG press
      userInputState.buttonPressStartTimeMillis = -1;
    }
  }

  // if here, fallback is IDLE
  userInputState.state = IDLE;
}

void userInputInit() {
  if (userButton.begin() == false) {
    Log.error("Device did not acknowledge! Freezing.");
  }
  Log.error("Button acknowledged.");
}