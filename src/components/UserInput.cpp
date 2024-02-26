
#include "UserInput.h"

String SHORT_BUTTON_COMMAND = String("short");
String LONG_BUTTON_COMMAND = String("long");

// Based on the physical button, we derive one of three
// input states
UserInputState userInputState;

QwiicButton userButton;

// How long the button has to be pressed before it is considered
// a long press
float BUTTON_LONG_PRESS_DURATION_MILLIS = 2000;

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

  // no physical inputs.. now check for commands from MQTT or BLE ...
  char* incomingCommand = NULL;  

  char* _incomingMQTTCommand = checkForMQTTCommand();
  if (_incomingMQTTCommand != NULL) {
    incomingCommand = _incomingMQTTCommand; 
  } else {
    char* _incomingBLECommand = checkForBLECommand();
    if (_incomingBLECommand != NULL) {
      incomingCommand = _incomingBLECommand; 
    }
  }

  if (incomingCommand != NULL) {
    String incomingCommandString = String(incomingCommand);
    
    publishParticleLog("incomingCommand", incomingCommandString);
    Log.error("incomingCommand: " + String(incomingCommandString));

    if (incomingCommandString.startsWith(SHORT_BUTTON_COMMAND)) {
      publishParticleLog("incomingCommand", "SHORT_PRESS detected");

      userInputState.state = SHORT_PRESS;
      userInputState.buttonPressStartTimeMillis = -1;
      userInputState.lastUserInteractionTimeMillis = nowTimeMillis;
    
      return;
    } else 
    if (incomingCommandString.startsWith(LONG_BUTTON_COMMAND)) {
      publishParticleLog("mqttCommand", "LONG_PRESS detected");

      userInputState.state = LONG_PRESS;
      userInputState.buttonPressStartTimeMillis = 0; 
      userInputState.lastUserInteractionTimeMillis = nowTimeMillis;
    
      return;
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