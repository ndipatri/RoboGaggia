#ifndef BLUETOOTH_H
#define BLUETOOTH_H

#include "Common.h"

const BleUuid uartServiceUUID("6E400001-B5A3-F393-E0A9-E50E24DCCA9E");
const BleUuid rxUUID("6E400002-B5A3-F393-E0A9-E50E24DCCA9E");
const BleUuid txUUID("6E400003-B5A3-F393-E0A9-E50E24DCCA9E");

void sendMessageOverBLE(const char* message);

void bluetoothInit();

char* checkForBLECommand();

void onDataReceived(const uint8_t* data, size_t len, const BlePeerDevice& peer, void* context);

#endif