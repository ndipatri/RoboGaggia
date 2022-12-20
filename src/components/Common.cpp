#include "Common.h"

// Slows down the main loop interval so we can monitor certain behaviors.. also allows
// for loop-level debug logs to be sent to Particle Cloud
boolean isInTestMode = false;

void publishParticleLog(String group, String message) {
    if (PLATFORM_ID == PLATFORM_ARGON && isInTestMode) {
        Particle.publish(group, message, 60, PUBLIC);
        Log.error(message);
    }
}

int turnOnTestMode(String _na) {

    Particle.publish("config", "testMode turned ON", 60, PUBLIC);
    isInTestMode = true;

    return 1;
}

int turnOffTestMode(String _na) {

    Particle.publish("config", "testMode turned OFF", 60, PUBLIC);
    isInTestMode = false;

    return 1;
}

int enterDFUMode(String _na) {

    Particle.publish("config", "entering DFU mode...", 60, PUBLIC);

    System.dfu(RESET_NO_WAIT);

    return 1;
}

void commonInit() {
  Particle.variable("isInTestMode",  isInTestMode);
  Particle.function("turnOnTestMode", turnOnTestMode);
  Particle.function("turnOffTestMode", turnOffTestMode);
  Particle.function("enterDFUMode", enterDFUMode);
}