/*
 * This sketch is part of ServoMote
 * See https://docs.servomote.app for more information
 * 
 * Servo Control with BLE
 *
 * Controls servo motors via Bluetooth Low Energy (BLE).
 * Send commands in format: "servo_id:angle" (e.g., "1:90")
 *
 */

#include <ESP32Servo.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

#define DEVICE_NAME "ServoMote"

const int NUM_SERVOS = 3;
const int SERVO_PINS[NUM_SERVOS] = {2, 3, 4};
const int MIN_ANGLE = 0;
const int MAX_ANGLE = 180;

Servo servos[NUM_SERVOS];
int servoAngles[NUM_SERVOS] = {-1, -1, -1};

BLEServer* pServer = NULL;
BLECharacteristic* pCharacteristic = NULL;
bool deviceConnected = false;
bool oldDeviceConnected = false;

class ServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) {
    deviceConnected = true;
    Serial.println("Client connected");
  }

  void onDisconnect(BLEServer* pServer) {
    deviceConnected = false;
    Serial.println("Client disconnected");
  }
};

class CharacteristicCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic* pCharacteristic) {
    String value = pCharacteristic->getValue().c_str();

    if (value.length() > 0) {
      Serial.print("Received: ");
      Serial.println(value);

      int colonIndex = value.indexOf(':');
      if (colonIndex > 0) {
        // Set command: servo_id:angle
        int servoId = value.substring(0, colonIndex).toInt();
        int angle = value.substring(colonIndex + 1).toInt();

        if (servoId >= 1 && servoId <= NUM_SERVOS) {
          angle = constrain(angle, MIN_ANGLE, MAX_ANGLE);
          int index = servoId - 1;
          servoAngles[index] = angle;
          servos[index].write(angle);

          Serial.print("Servo ");
          Serial.print(servoId);
          Serial.print(" set to ");
          Serial.print(angle);
          Serial.println(" degrees");

          String response = "OK:" + String(servoId) + ":" + String(angle);
          pCharacteristic->setValue(response.c_str());
          pCharacteristic->notify();
        } else {
          Serial.println("Invalid servo ID (use 1-3)");
          pCharacteristic->setValue("ERROR:Invalid servo ID");
          pCharacteristic->notify();
        }
      } else {
        // Get command: servo_id
        int servoId = value.toInt();
        if (servoId >= 1 && servoId <= NUM_SERVOS) {
          int index = servoId - 1;
          String response = "OK:" + String(servoId) + ":" + String(servoAngles[index]);
          pCharacteristic->setValue(response.c_str());
          pCharacteristic->notify();
        } else {
          pCharacteristic->setValue("ERROR:Invalid servo ID");
          pCharacteristic->notify();
        }
      }
    }
  }
};

void setup() {                                                                                                                                                                                          
  Serial.begin(9600);
  delay(1000);
  Serial.println("Servo Control with BLE");

  // Initialize servos
  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  for (int i = 0; i < NUM_SERVOS; i++) {
    servos[i].setPeriodHertz(50);
    servos[i].attach(SERVO_PINS[i], 500, 2400);
    Serial.print("Servo ");
    Serial.print(i + 1);
    Serial.print(" on GPIO ");
    Serial.println(SERVO_PINS[i]);
  }

  // Initialize BLE
  BLEDevice::init(DEVICE_NAME);
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new ServerCallbacks());

  BLEService* pService = pServer->createService(SERVICE_UUID);
  pCharacteristic = pService->createCharacteristic(
    CHARACTERISTIC_UUID,
    BLECharacteristic::PROPERTY_READ |
    BLECharacteristic::PROPERTY_WRITE |
    BLECharacteristic::PROPERTY_NOTIFY
  );

  pCharacteristic->addDescriptor(new BLE2902());
  pCharacteristic->setCallbacks(new CharacteristicCallbacks());
  pCharacteristic->setValue("Ready");

  pService->start();

  BLEAdvertising* pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->setName(DEVICE_NAME);
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);  // Helps with iPhone connection
  BLEDevice::startAdvertising();

  Serial.print("Service UUID: ");
  Serial.println(SERVICE_UUID);

  Serial.println("BLE advertising started");
  Serial.println("Send commands as: servo_id:angle (e.g., 1:90)");
}

void loop() {
  // Handle reconnection
  if (!deviceConnected && oldDeviceConnected) {
    delay(500);
    pServer->startAdvertising();
    Serial.println("Restarted advertising");
    oldDeviceConnected = deviceConnected;
  }

  if (deviceConnected && !oldDeviceConnected) {
    oldDeviceConnected = deviceConnected;
  }

  delay(10);
}
