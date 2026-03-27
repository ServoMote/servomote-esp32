/*
 * This sketch is part of ServoMote
 * See https://docs.servomote.app for more information
 *
 * Servo Control with BLE using PCA9685
 *
 * Controls servo motors via Bluetooth Low Energy (BLE) using a PCA9685 PWM driver.
 * Send commands in format: "servo_id:angle" (e.g., "1:90")
 *
 * Connections (Seeed XIAO ESP32-C6):
 *   PCA9685 VCC  -> 3.3V
 *   PCA9685 GND  -> GND
 *   PCA9685 SDA  -> D4 (GPIO 22)
 *   PCA9685 SCL  -> D5 (GPIO 23)
 *   PCA9685 V+   -> 5-6V servo power supply
 *   PCA9685 GND  -> Servo power supply GND (common ground)
 */

#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

// I2C pins for Seeed XIAO ESP32-C3 (D4=SDA, D5=SCL)
#define SDA_PIN 6
#define SCL_PIN 7

// PCA9685 settings
#define PCA9685_ADDRESS 0x40
#define SERVO_FREQ 50  // 50 Hz for standard servos

// Servo pulse length limits (in microseconds)
#define SERVO_MIN_US 500
#define SERVO_MAX_US 2400

// BLE UUIDs - same as original
#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

#define DEVICE_NAME "ServoMote"

const int NUM_SERVOS = 16;  // PCA9685 supports 16 channels
const int MIN_ANGLE = 0;
const int MAX_ANGLE = 180;

Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver(PCA9685_ADDRESS);

int servoAngles[NUM_SERVOS] = {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1};

BLEServer* pServer = NULL;
BLECharacteristic* pCharacteristic = NULL;
bool deviceConnected = false;
bool oldDeviceConnected = false;

// Convert angle (0-180) to PWM value
uint16_t angleToPWM(int angle) {
  int pulseUs = map(angle, 0, 180, SERVO_MIN_US, SERVO_MAX_US);
  return (uint16_t)(pulseUs * 4096 / 20000);
}

// Set servo angle
void setServoAngle(uint8_t channel, int angle) {
  if (channel >= NUM_SERVOS) return;
  angle = constrain(angle, MIN_ANGLE, MAX_ANGLE);
  servoAngles[channel] = angle;
  uint16_t pwmValue = angleToPWM(angle);
  pwm.setPWM(channel, 0, pwmValue);
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
        // Set command: servo_id:angle
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

          String response = "OK:" + String(servoId) + ":" + String(angle);
          pCharacteristic->setValue(response.c_str());
          pCharacteristic->notify();
        } else {
          Serial.println("Invalid servo ID (use 1-16)");
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
  Serial.println("Servo Control with BLE (PCA9685)");

  // Initialize I2C with custom pins for ESP32-C6
  Wire.begin(SDA_PIN, SCL_PIN);

  // Initialize PCA9685
  pwm.begin();
  pwm.setOscillatorFrequency(27000000);
  pwm.setPWMFreq(SERVO_FREQ);
  delay(10);

  Serial.println("PCA9685 initialized - 16 servo channels available");

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
  Serial.println("Device name: ServoController");
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
