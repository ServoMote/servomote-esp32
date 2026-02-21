/*
 * This sketch is part of ServoMote
 * See https://docs.servomote.app for more information
 *
 * Servo Control with BLE using M5Stack 8Servo Unit (STM32F030)
 *
 * Controls servo motors via Bluetooth Low Energy (BLE) using an M5Stack 8-Channel
 * Servo Driver Unit with STM32F030 controller, communicating over I2C.
 * Send commands in format: "servo_id:angle" (e.g., "1:90")
 *
 * Connections (ESP32):
 *   8Servo Unit VCC -> 3.3V
 *   8Servo Unit GND -> GND
 *   8Servo Unit SDA -> SDA_PIN (GPIO 21 default)
 *   8Servo Unit SCL -> SCL_PIN (GPIO 22 default)
 *   8Servo Unit V+  -> 5-6V servo power supply
 *   8Servo Unit GND -> Servo power supply GND (common ground)
 */

#include <Wire.h>
#include <M5_UNIT_8SERVO.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include "services/gap/ble_svc_gap.h"
#include "host/ble_gap.h"

// I2C pins (adjust for your ESP32 board)
#define SDA_PIN 13
#define SCL_PIN 15

// BLE UUIDs - same as original
#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

#define DEVICE_NAME "ServoMote"

const int NUM_SERVOS = 8;  // M5Stack 8Servo Unit supports 8 channels
const int MIN_ANGLE = 0;
const int MAX_ANGLE = 180;

M5_UNIT_8SERVO unit_8servo;

int servoAngles[NUM_SERVOS] = {90, 90, 90, 90, 90, 90, 90, 90};

BLEServer* pServer = NULL;
BLECharacteristic* pCharacteristic = NULL;
bool deviceConnected = false;
bool oldDeviceConnected = false;

// Set servo angle via M5Stack 8Servo Unit
void setServoAngle(uint8_t channel, int angle) {
  if (channel >= NUM_SERVOS) return;
  angle = constrain(angle, MIN_ANGLE, MAX_ANGLE);
  servoAngles[channel] = angle;
  unit_8servo.setServoAngle(channel, angle);
}

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
        int servoId = value.substring(0, colonIndex).toInt();
        int angle = value.substring(colonIndex + 1).toInt();

        if (servoId >= 1 && servoId <= NUM_SERVOS) {
          angle = constrain(angle, MIN_ANGLE, MAX_ANGLE);
          int index = servoId - 1;  // Convert to 0-based index
          setServoAngle(index, angle);

          Serial.print("Servo ");
          Serial.print(servoId);
          Serial.print(" set to ");
          Serial.print(angle);
          Serial.println(" degrees");

          // Send confirmation back
          String response = "OK:" + String(servoId) + ":" + String(angle);
          pCharacteristic->setValue(response.c_str());
          pCharacteristic->notify();
        } else {
          Serial.println("Invalid servo ID (use 1-8)");
          pCharacteristic->setValue("ERROR:Invalid servo ID");
          pCharacteristic->notify();
        }
      } else {
        Serial.println("Invalid format (use servo_id:angle)");
        pCharacteristic->setValue("ERROR:Invalid format");
        pCharacteristic->notify();
      }
    }
  }
};

void setup() {
  Serial.begin(9600);
  delay(1000);
  Serial.println("Servo Control with BLE (M5Stack 8Servo Unit)");

  // Initialize I2C
  Wire.begin(SDA_PIN, SCL_PIN);

  // Initialize M5Stack 8Servo Unit
  while (!unit_8servo.begin(&Wire, SDA_PIN, SCL_PIN, M5_UNIT_8SERVO_DEFAULT_ADDR)) {
    Serial.println("M5Stack 8Servo Unit not found, retrying...");
    delay(1000);
  }

  // Set all pins to servo control mode
  unit_8servo.setAllPinMode(SERVO_CTL_MODE);

  // Set all servos to center position
  for (int i = 0; i < NUM_SERVOS; i++) {
    setServoAngle(i, servoAngles[i]);
  }
  Serial.println("M5Stack 8Servo Unit initialized - 8 servo channels available");

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
  pAdvertising->setMinPreferred(0x06);

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
