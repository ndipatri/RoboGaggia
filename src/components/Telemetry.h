
#ifndef TELEMETRY_H
#define TELEMETRY_H

#include "Network.h"
#include "Bluetooth.h"
#include "Common.h"
#include "State.h"
#include "tiny-collections.h"

void sendTelemetryUpdateNow();

// Return value indicates telemetry measurement was taken
void calculateAndSendTelemetryIfNecessary();

#endif