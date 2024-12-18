#include "Telemetry.h"

using namespace tc; // Import tc::* into the global namespace

// We want the telemetry send rate to be independent of all 
// other proceses in gaggia, so track it separately...
float nextTelemetrySendMillis = -1;

String lastMessageSentToCloud = "";

int SEND_TELEMETRY_INTERVAL_MILLIS = 250; 

void sendTelemetry(boolean force) {

  Telemetry telemetry;

  telemetry.id = currentGaggiaState->state;
  telemetry.stateName = getStateName(currentGaggiaState->state);
  telemetry.measuredWeightGrams = scaleState.measuredWeight - scaleState.tareWeight;
  telemetry.measuredPressureBars = waterPumpState.measuredPressureInBars;
  telemetry.pumpDutyCycle = waterPumpState.pumpDutyCycle;
  telemetry.flowRateGPS = waterPumpState.flowRateGPS;
  telemetry.brewTempC = heaterState.measuredTemp;
  telemetry.shotsUntilBackflush = shotsUntilBackflush();
  telemetry.totalShots = readTotalBrewCount();

  if (isHeaterOn()) {
    telemetry.boilerState = 1; 
  } else {
    telemetry.boilerState = 0; 
  }

    // We encode different values based on state...
  if (currentGaggiaState->state == BACKFLUSH_CYCLE_1 ||
      currentGaggiaState->state == BACKFLUSH_CYCLE_2) {

      // weight is mapped to 'current pass'
      telemetry.measuredWeightGrams = (long)(currentGaggiaState->counter/2)+1;

      // pressure is maped to the 'target pass count'
      telemetry.measuredPressureBars = (long)(currentGaggiaState->targetCounter)/2;
  }

  // only way this send will succeed
  char measuredWeightGramsBuf[256];
  snprintf(measuredWeightGramsBuf, 
           sizeof(measuredWeightGramsBuf), 
           "%.1lf", 
           (float)telemetry.measuredWeightGrams);

  // Now we add the 'target weight' for our composite weight value...
  char targetWeightGramsBuf[256];
  snprintf(targetWeightGramsBuf, 
           sizeof(targetWeightGramsBuf), 
           "%.1lf", 
           (float)scaleState.targetWeight);  

  String weight = measuredWeightGramsBuf + String(":") + targetWeightGramsBuf;

  char measuredTempBuf[256];
  snprintf(measuredTempBuf, 
           sizeof(measuredTempBuf), 
           "%.1lf", 
           (float)telemetry.brewTempC);

  // Now we add the 'target temp' for our composite temp value...
  char targetTempGramsBuf[256];
  snprintf(targetTempGramsBuf, 
           sizeof(targetTempGramsBuf), 
           "%.1lf", 
           (float)heaterState.targetTemp);  

  String temp = measuredTempBuf + String(":") + targetTempGramsBuf;

  String messageToSendToCloud =             
    String(telemetry.stateName) + String(", ") + 
    String(weight) + String(", ") + 
    String((int)floor(telemetry.measuredPressureBars)) + String(", ") +  
    String((int)floor(telemetry.pumpDutyCycle)) + String(", ") +
    String((int)floor(telemetry.flowRateGPS)) + String(", ") +
    String(temp) + String(", ") + 
    String(telemetry.shotsUntilBackflush) + String(", ") +
    String(telemetry.totalShots) + String(", ") +
    String(telemetry.boilerState);

    if (force || (!messageToSendToCloud.equals(lastMessageSentToCloud))) {
      Log.error(String(millis()) + String(":") + messageToSendToCloud);

      sendMessageOverBLE(messageToSendToCloud);

      lastMessageSentToCloud = messageToSendToCloud;
    }
}

void sendTelemetryIfNecessary(boolean force) {
  if (force || (millis() >= nextTelemetrySendMillis)) {
    sendTelemetry(force);
  
    nextTelemetrySendMillis = millis() + SEND_TELEMETRY_INTERVAL_MILLIS;
  }
}