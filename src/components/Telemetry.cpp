#include "Telemetry.h"

using namespace tc; // Import tc::* into the global namespace

int TELEMETRY_SAMPLE_PERIOD_MILLIS = 200; 

// This is the interval of time between when we send telemetry values
// to adafruit.io.  Adafruit IO will block this client if we send
// these more frequently that once every second.
// This * TELEMETRY_SAMPLE_PERIOD_MILLIS (200ms) must be <= 1200!
int TELEMETRY_SEND_INTERVAL = 6; 

// The telemetry that we send to the cloud is a discrete window average.
// The window size in seconds is defined by 
// TELEMETRY_SEND_INTERVAL * TELEMETRY_SAMPLE_PERIOD_MILLIS
// It emits a new telemetry every ^^ seconds...

vector<Telemetry> telemetryHistory;

String lastMessageSentToCloud = "";

float nextTelemetrySampleMillis = -1;

void sendTelemetry(boolean force) {

    Telemetry telemetry;
    telemetry.id = currentGaggiaState->state;
    telemetry.description = "PID(" + 
                              String(pressure_PID_kP) + ":" +
                              String(pressure_PID_kI) + ":" +
                              String(pressure_PID_kD) + ")";
    telemetry.stateName = getStateName(currentGaggiaState->state);
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
    if (force || telemetryHistory.size() >= TELEMETRY_SEND_INTERVAL) {
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
        averageTelemetry.id = currentGaggiaState->state;
        averageTelemetry.stateName = getStateName(currentGaggiaState->state);
        averageTelemetry.measuredWeightGrams = weightSum/TELEMETRY_SEND_INTERVAL;
        averageTelemetry.measuredPressureBars = barsSum/TELEMETRY_SEND_INTERVAL;
        averageTelemetry.pumpDutyCycle = dutyCycleSum/TELEMETRY_SEND_INTERVAL;
        averageTelemetry.flowRateGPS = flowRateSum/TELEMETRY_SEND_INTERVAL;
        averageTelemetry.brewTempC = brewTempSum/TELEMETRY_SEND_INTERVAL;

        // This is a simplistic, finite queue that simply spans the duration
        // of telementry send events...
        telemetryHistory.clear();

      // only way this send will succeed
      #ifdef AIO_USERNAME
        char measuredWeightGramsBuf[256];
        snprintf(measuredWeightGramsBuf, 
                 sizeof(measuredWeightGramsBuf), 
                 "%.1lf", 
                 (float)averageTelemetry.measuredWeightGrams);

        String messageToSendToCloud =             
            String(averageTelemetry.stateName) + String(", ") + 
            String(averageTelemetry.description) + String(", ") + 
            String(measuredWeightGramsBuf) + String(", ") + 
            String((int)floor(averageTelemetry.measuredPressureBars)) + String(", ") +  
            String((int)floor(averageTelemetry.pumpDutyCycle)) + String(", ") +
            String((int)floor(averageTelemetry.flowRateGPS)) + String(", ") +
            String((int)floor(averageTelemetry.brewTempC));        

        if (!messageToSendToCloud.equals(lastMessageSentToCloud)) {
          Log.error(String(millis()) + String(":") + messageToSendToCloud);

          sendMessageToCloud(messageToSendToCloud);
          
          String messageToSendToBluetooth =             
            String(averageTelemetry.id) + String(", ") + 
            String(measuredWeightGramsBuf) + String(", ") + 
            String((int)floor(averageTelemetry.measuredPressureBars)) + String(", ") +  
            String((int)floor(averageTelemetry.pumpDutyCycle)) + String(", ") +
            String((int)floor(averageTelemetry.flowRateGPS)) + String(", ") +
            String((int)floor(averageTelemetry.brewTempC));   

          sendMessageOverBLE(messageToSendToCloud);
        
          lastMessageSentToCloud = messageToSendToCloud;
        }

      #endif
    }
  }

void sendTelemetryUpdateNow() {
  sendTelemetry(true);
}

void calculateAndSendTelemetryIfNecessary() {
  if (millis() > nextTelemetrySampleMillis) {
    sendTelemetry(false);

    nextTelemetrySampleMillis = millis() + TELEMETRY_SAMPLE_PERIOD_MILLIS;
  }
}