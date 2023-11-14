#include "Bluetooth.h"

BleCharacteristic txCharacteristic("tx", BleCharacteristicProperty::NOTIFY, txUUID, uartServiceUUID);
BleCharacteristic rxCharacteristic("rx", BleCharacteristicProperty::WRITE_WO_RSP, rxUUID, uartServiceUUID, onDataReceived, NULL);

void bluetoothInit() {
  BLE.on();

  BLE.addCharacteristic(txCharacteristic);
  BLE.addCharacteristic(rxCharacteristic);

  BleAdvertisingData data;
  data.appendServiceUUID(uartServiceUUID);
  BLE.advertise(&data);
}

void onDataReceived(const uint8_t* data, size_t len, const BlePeerDevice& peer, void* context) {
    // Log.trace("Received data from: %02X:%02X:%02X:%02X:%02X:%02X:", peer.address()[0], peer.address()[1], peer.address()[2], peer.address()[3], peer.address()[4], peer.address()[5]);

    for (size_t ii = 0; ii < len; ii++) {
        Serial.write(data[ii]);
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
