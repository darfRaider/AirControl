#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>

// GoPro BLE UUID definitions
#define GOPRO_SERVICE_UUID        "0000fea6-0000-1000-8000-00805f9b34fb" 
#define GOPRO_COMMAND_CHAR_UUID   "b5f90072-aa8d-11e3-9046-0002a5d5c51b"

const int BUTTON_PIN = 27; // Change based on your exact FireBeetle model pinout
bool doConnect = false;
bool connected = false;
BLERemoteCharacteristic* pRemoteCharacteristic;
BLEAdvertisedDevice* myDevice;

// Custom Security Callback Required by GoPro
class MySecurity : public BLESecurityCallbacks {
  uint32_t onPassKeyRequest() { return 0; }
  void onPassKeyNotify(uint32_t pass_key) {}
  bool onConfirmPIN(uint32_t pin) { return true; }
  bool onSecurityRequest() { return true; }
  void onAuthenticationComplete(esp_ble_auth_cmpl_t cmpl) {
    Serial.println("Pairing and Bonding Complete!");
  }
};

// Scanner callback to locate the camera
class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(BLEUUID(GOPRO_SERVICE_UUID))) {
      BLEDevice::getScan()->stop();
      myDevice = new BLEAdvertisedDevice(advertisedDevice);
      doConnect = true;
    }
  }
};

bool connectToServer() {
  Serial.print("Forming a connection to ");
  Serial.println(myDevice->getAddress().toString().c_str());
  
  BLEClient* pClient = BLEDevice::createClient();
  
  // CRITICAL STEP: Set encryption level for GoPro pairing bonding
  // pClient->setEncryptionLevel(ESP_BLE_SEC_ENCRYPT);
  
  pClient->connect(myDevice);
  Serial.println("Connected to GoPro");

  BLERemoteService* pRemoteService = pClient->getService(BLEUUID(GOPRO_SERVICE_UUID));
  if (pRemoteService == nullptr) {
    pClient->disconnect();
    return false;
  }

  pRemoteCharacteristic = pRemoteService->getCharacteristic(BLEUUID(GOPRO_COMMAND_CHAR_UUID));
  if (pRemoteCharacteristic == nullptr) {
    pClient->disconnect();
    return false;
  }
  
  connected = true;
  return true;
}

void sendGoProCommand(uint8_t* cmd, size_t len) {
  if (connected && pRemoteCharacteristic != nullptr) {
    pRemoteCharacteristic->writeValue(cmd, len, true);
    Serial.println("Command Sent!");
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  
  // Initialize BLE Device
  BLEDevice::init("FireBeetleRemote");
  
  // Assign Security Callbacks directly to the Device
  BLEDevice::setSecurityCallbacks(new MySecurity());
  
  // NEW V3.X COMPLIANT SECURITY STRUCTURE:
  // Instead of setting encryption level on BLEDevice, we configure it entirely 
  // via the BLESecurity object using bool arguments (bonding, mitm, secure_connections)
  BLESecurity *pSecurity = new BLESecurity();
  pSecurity->setAuthenticationMode(true, false, true); // (bonding = true, mitm = false, sc = true)
  pSecurity->setCapability(ESP_IO_CAP_NONE);

  // Initialize and launch the BLE Scanner
  BLEScan* pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setInterval(1349);
  pBLEScan->setWindow(449);
  pBLEScan->setActiveScan(true);
  pBLEScan->start(5, false);
}


void loop() {
  if (doConnect == true) {
    if (connectToServer()) {
      Serial.println("GoPro Remote Ready.");
    } else {
      Serial.println("Failed to connect.");
    }
    doConnect = false;
  }

  // Detect button press (Low state means pressed due to INPUT_PULLUP)
  if (connected && digitalRead(BUTTON_PIN) == LOW) {
    delay(50); // Simple Debounce
    if (digitalRead(BUTTON_PIN) == LOW) {
      Serial.println("Button Pressed: Triggering Shutter...");
      
      // Open GoPro hex command array to trigger Record/Shutter Start
      uint8_t shutter_start[] = {0x03, 0x01, 0x01, 0x01};
      sendGoProCommand(shutter_start, sizeof(shutter_start));
      
      // Wait for button release to prevent spamming
      while(digitalRead(BUTTON_PIN) == LOW); 
    }
  }
  delay(10);
}
