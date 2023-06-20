#include "Telemetry.h"

using namespace tc; // Import tc::* into the global namespace

// This is the interval of time between when we send telemetry values
// to adafruit.io.  Adafruit IO will block this client if we send
// these more frequently that once every second.
// This * TELEMETRY_COLLECT_PERIOD_MILLIS must be >= 1200!
int TELEMETRY_SEND_INTERVAL = 6; 

vector<Telemetry> telemetryHistory;

void calculateAndSendTelemetryIfNecessary() {

    Telemetry telemetry;
    telemetry.description = "PID(" + 
                              String(pressure_PID_kP) + ":" +
                              String(pressure_PID_kI) + ":" +
                              String(pressure_PID_kD) + ")";
    telemetry.stateName = getStateName(currentGaggiaState.state);
    telemetry.measuredWeightGrams = abs(scaleState.measuredWeight - scaleState.tareWeight);
    telemetry.measuredPressureBars = waterPumpState.measuredPressureInBars;
    telemetry.pumpDutyCycle = waterPumpState.pumpDutyCycle;
    telemetry.flowRateGPS = waterPumpState.flowRateGPS;
    telemetry.brewTempC = heaterState.measuredTemp;

    // Log.error(String(telemetry.measuredWeightGrams) + "," +
    //   String(telemetry.measuredPressureBars) + "," +
    //   String(telemetry.pumpDutyCycle) + "," +
    //   String(telemetry.flowRateGPS));
  
    // build up a queue...
    telemetryHistory.push_back(telemetry);

    // and if we have to send telemetry, we average queue and send results ...
    if (telemetryHistory.size() >= TELEMETRY_SEND_INTERVAL) {
        double weightSum = 0;
        double barsSum = 0;
        double dutyCycleSum = 0;
        double flowRateSum = 0;
        double brewTempSum = 0;
        for (auto dataPoint: telemetryHistory) {
          weightSum += dataPoint.measuredWeightGrams;
          barsSum += dataPoint.measuredPressureBars;
          dutyCycleSum += dataPoint.pumpDutyCycle;
          flowRateSum += dataPoint.flowRateGPS;
          brewTempSum += dataPoint.brewTempC;
        }

        Telemetry averageTelemetry;
          // will use last value
        averageTelemetry.stateName = getStateName(currentGaggiaState.state);
        averageTelemetry.measuredWeightGrams = weightSum/TELEMETRY_SEND_INTERVAL;
        averageTelemetry.measuredPressureBars = barsSum/TELEMETRY_SEND_INTERVAL;
        averageTelemetry.pumpDutyCycle = dutyCycleSum/TELEMETRY_SEND_INTERVAL;
        averageTelemetry.flowRateGPS = flowRateSum/TELEMETRY_SEND_INTERVAL;
        averageTelemetry.brewTempC = brewTempSum/TELEMETRY_SEND_INTERVAL;


        Log.error("avg:" + String(averageTelemetry.measuredWeightGrams) + "," +
          String(averageTelemetry.measuredPressureBars) + "," +
          String(averageTelemetry.pumpDutyCycle) + "," +
          String(averageTelemetry.flowRateGPS));

        // This is a simplistic, finite queue that simply spans the duration
        // of telementry send events...
        telemetryHistory.clear();

      // only way this send will succeed
      #ifdef AIO_USERNAME
        sendMessageToCloud(
            String(averageTelemetry.stateName) + String(", ") + 
            String(averageTelemetry.description) + String(", ") + 
            String(averageTelemetry.measuredWeightGrams) + String(", ") + 
            String(averageTelemetry.measuredPressureBars) + String(", ") +  
            String(averageTelemetry.pumpDutyCycle) + String(", ") +
            String(averageTelemetry.flowRateGPS)  + String(", ") +
            String(averageTelemetry.brewTempC));
      #endif

  }
}