#include "Telemetry.h"

float nextTelemetryMillis = -1;

// This is the interal of time between when we send telemetry values
// to adafruit.io.  Adafruit IO will block this client if we send
// these more frequently that once every second.
int TELEMETRY_PERIOD_MILLIS = 1200; 

boolean TELEMETRY_ENABLED = true;

void sendTelemetry() {

  Telemetry telemetry;
  telemetry.stateName = getStateName(currentGaggiaState.state);
  telemetry.measuredWeightGrams = scaleState.measuredWeight - scaleState.tareWeight;
  telemetry.measuredPressureBars = waterPumpState.measuredPressureInBars;
  telemetry.pumpDutyCycle = waterPumpState.pumpDutyCycle;
  telemetry.flowRateGPS = waterPumpState.flowRateGPS;
  telemetry.brewTempC = heaterState.measuredTemp;

  if (TELEMETRY_ENABLED) {

    Log.error("New Telemetry: (" + 
        String(telemetry.stateName) + "," +
        String(telemetry.measuredWeightGrams) + "," +
        String(telemetry.measuredPressureBars) + "," +
        String(telemetry.pumpDutyCycle) + "," +
        String(telemetry.flowRateGPS) + "," +
        String(telemetry.brewTempC) + ")" 
      );
    
    sendMessageToCloud(
        String(telemetry.stateName) + String(", ") + 
        String(telemetry.measuredWeightGrams) + String(", ") + 
        String(telemetry.measuredPressureBars) + String(", ") +  
        String(telemetry.pumpDutyCycle) + String(", ") +
        String(telemetry.flowRateGPS)  + String(", ") +
        String(telemetry.brewTempC));
  }
}