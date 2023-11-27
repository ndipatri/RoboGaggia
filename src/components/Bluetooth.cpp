#include "Bluetooth.h"

BleCharacteristic txCharacteristic("tx", BleCharacteristicProperty::NOTIFY, txUUID, uartServiceUUID);
BleCharacteristic rxCharacteristic("rx", BleCharacteristicProperty::WRITE_WO_RSP, rxUUID, uartServiceUUID, onDataReceived, NULL);

char* receivedBLEMessage;

void bluetoothInit() {
  BLE.on();

  BLE.addCharacteristic(txCharacteristic);
  BLE.addCharacteristic(rxCharacteristic);

  BleAdvertisingData data;
  data.appendServiceUUID(uartServiceUUID);
  BLE.advertise(&data);
}

void onDataReceived(const uint8_t* data, size_t len, const BlePeerDevice& peer, void* context) {
    for (size_t ii = 0; ii < len; ii++) {
        Serial.write(data[ii]);
    }

    receivedBLEMessage = (char*)data;
}

char* checkForBLECommand() {
  if (receivedBLEMessage != NULL) {
    char* _receivedBLEMessage = receivedBLEMessage;
    receivedBLEMessage = NULL;

    return _receivedBLEMessage;
  } else {
    return NULL;
  }
}

void sendMessageOverBLE(const char* message) {
  if (BLE.connected()) {
    String messageString = String(message);
    txCharacteristic.setValue(messageString);
  }
}

char* checkForIncomingCommand() {
  return "hi";
}
