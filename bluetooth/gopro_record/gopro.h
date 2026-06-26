#ifndef SENSORS_H
#define SENSORS_H

#include <Arduino.h>

// GoPro BLE UUID definitions
#define GOPRO_SERVICE_UUID        "0000fea6-0000-1000-8000-00805f9b34fb" 
#define GOPRO_COMMAND_CHAR_UUID   "b5f90072-aa8d-11e3-9046-0002a5d5c51b"

float readTemperature();
float readHumidity();

bool doConnect = false;
bool connected = false;
BLERemoteCharacteristic* pRemoteCharacteristic;
BLEAdvertisedDevice* myDevice;

class MySecurity : public BLESecurityCallbacks {
  uint32_t onPassKeyRequest() { return 0; }
  void onPassKeyNotify(uint32_t pass_key) {}
  bool onConfirmPIN(uint32_t pin) { return true; }
  bool onSecurityRequest() { return true; }
  void onAuthenticationComplete(esp_ble_auth_cmpl_t cmpl) {
    Serial.println("Pairing and Bonding Complete!");
  }
};

class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(BLEUUID(GOPRO_SERVICE_UUID))) {
      BLEDevice::getScan()->stop();
      myDevice = new BLEAdvertisedDevice(advertisedDevice);
      doConnect = true;
    }
  }
};

void sendGoProCommand(uint8_t* cmd, size_t len) {
  if (connected && pRemoteCharacteristic != nullptr) {
    pRemoteCharacteristic->writeValue(cmd, len, true);
    Serial.println("Command Sent!");
  }
}

void startGroProRecoriding(){
    uint8_t shutter_start[] = {0x03, 0x01, 0x01, 0x01};
    sendGoProCommand(shutter_start, sizeof(shutter_start));
}

void stopGoProRecoding(){
    uint8_t shutter_stop[] = {0x03, 0x01, 0x01, 0x00};
    sendGoProCommand(shutter_stop, sizeof(shutter_stop));
}

#endif