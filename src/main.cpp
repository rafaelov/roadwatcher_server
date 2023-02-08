#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>

#include <Adafruit_MPU6050.h>

#define SERVICE_UUID        "ADAF0002-C332-42A8-93BD-25E905756CB8"
#define CHARACTERISTIC_UUID "ADAF0201-C332-42A8-93BD-25E905756CB8"

// MPU6050 Variables
Adafruit_MPU6050 mpu;
sensors_event_t acc, gyro, temp;

// Service variables
bool deviceConnected = false;
bool oldDeviceConnected = false;

BLEServer *pServer = NULL;
BLECharacteristic *pCharacteristic = NULL;

class MyServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer *pServer) override {
    deviceConnected = true;
  }

  void onDisconnect(BLEServer *pServer) override {
    deviceConnected = false;
  }
};

void initialize_mpu6050() {
  if (!mpu.begin()) {
    Serial.println("Failed to find MPU6050 chip.");
    while (true) {
      delay(10);
    }
  } else {
    Serial.println("MPU6050 started.");
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println("Starting BLE work!");
  initialize_mpu6050();

  BLEDevice::init("RoadWatcher");

  // Create server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Create service
  BLEService *pService = pServer->createService(SERVICE_UUID);
  
  // Create characteristc
  pCharacteristic = pService->createCharacteristic(
                                         CHARACTERISTIC_UUID,
                                         BLECharacteristic::PROPERTY_READ   |
                                         BLECharacteristic::PROPERTY_WRITE  |
                                         BLECharacteristic::PROPERTY_NOTIFY |
                                         BLECharacteristic::PROPERTY_INDICATE
                                        );

  pCharacteristic->addDescriptor(new BLE2902());

  pService->start();
  
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);  // functions that help with iPhone connections issue
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();
  Serial.println("Characteristic defined! Now you can read it in your phone!");
}

void loop() {
  if (deviceConnected) {
    mpu.getEvent(&acc, &gyro, &temp);
    pCharacteristic->setValue(acc.acceleration.z);
    pCharacteristic->notify();
    delay(100);
  }

  if (!deviceConnected && oldDeviceConnected) {
    delay(500);
    pServer->startAdvertising();
    Serial.println("Advertising started.");
    oldDeviceConnected = deviceConnected;
  }

  if (deviceConnected && !oldDeviceConnected) {
    oldDeviceConnected = deviceConnected;
  }
}