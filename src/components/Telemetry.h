
#ifndef TELEMETRY_H
#define TELEMETRY_H

#include "Network.h"
#include "Bluetooth.h"
#include "Common.h"
#include "State.h"
#include "tiny-collections.h"

void sendTelemetryIfNecessary(boolean force);

#endif