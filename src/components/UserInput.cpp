
#include "UserInput.h"

String SHORT_BUTTON_COMMAND = String("short");
String LONG_BUTTON_COMMAND = String("long");

// Based on the physical button, we derive one of three
// input states
UserInputState userInputState;

// How long the button has to be pressed before it is considered
// a long press
float BUTTON_LONG_PRESS_DURATION_MILLIS = 2000;

// This can be considered an 'auto-off' feature as the HOME state represents
// effectively NO BEHAVIOR (e.g. heater is off, no extraction, no telemetry being sent)
int RETURN_TO_HOME_INACTIVITY_MINUTES = 30;

void readUserInputState() {

  float nowTimeMillis = millis();

  char* incomingCommand = checkForBLECommand();

  if (incomingCommand != NULL) {
    String incomingCommandString = String(incomingCommand);
    
    publishParticleLog("incomingCommand", incomingCommandString);
    Log.error("incomingCommand: " + String(incomingCommandString));

    if (incomingCommandString.startsWith(SHORT_BUTTON_COMMAND)) {
      publishParticleLog("incomingCommand", "SHORT_PRESS detected");

      userInputState.state = SHORT_PRESS;
      userInputState.lastUserInteractionTimeMillis = nowTimeMillis;
    
      return;
    } else 
    if (incomingCommandString.startsWith(LONG_BUTTON_COMMAND)) {
      publishParticleLog("incomingCommand", "LONG_PRESS detected");

      userInputState.state = LONG_PRESS;
      userInputState.lastUserInteractionTimeMillis = nowTimeMillis;
    
      return;
    } 
  }

  // if here, fallback is IDLE
  userInputState.state = IDLE;
}

