#include "Telemetry.h"

float nextTelemetryMillis = -1;

// This is the interal of time between when we send telemetry values
// to adafruit.io.  Adafruit IO will block this client if we send
// these more frequently that once every second.
int TELEMETRY_PERIOD_MILLIS = 1200; 

void sendTelemetry() {

  Telemetry telemetry;
  telemetry.description = "PID(" + 
                            String(pressure_PID_kP) + ":" +
                            String(pressure_PID_kI) + ":" +
                            String(pressure_PID_kD) + ")";
  telemetry.stateName = getStateName(currentGaggiaState.state);
  telemetry.measuredWeightGrams = scaleState.measuredWeight - scaleState.tareWeight;
  telemetry.measuredPressureBars = waterPumpState.measuredPressureInBars;
  telemetry.pumpDutyCycle = waterPumpState.pumpDutyCycle;
  telemetry.flowRateGPS = waterPumpState.flowRateGPS;
  telemetry.brewTempC = heaterState.measuredTemp;

    #ifdef AIO_USERNAME

      Log.error("New Telemetry: (" + 
          String(telemetry.stateName) + "," +
          String(telemetry.description) + "," +
          String(telemetry.measuredWeightGrams) + "," +
          String(telemetry.measuredPressureBars) + "," +
          String(telemetry.pumpDutyCycle) + "," +
          String(telemetry.flowRateGPS) + "," +
          String(telemetry.brewTempC) + ")" 
        );
    
      sendMessageToCloud(
          String(telemetry.stateName) + String(", ") + 
          String(telemetry.description) + String(", ") + 
          String(telemetry.measuredWeightGrams) + String(", ") + 
          String(telemetry.measuredPressureBars) + String(", ") +  
          String(telemetry.pumpDutyCycle) + String(", ") +
          String(telemetry.flowRateGPS)  + String(", ") +
          String(telemetry.brewTempC));
    #endif
}