# ServoMote - ESP32 Sketches

This repository contains Arduino sketches for ESP32 boards that serve as the hardware backend for **ServoMote**, a Bluetooth Low Energy (BLE) servo control app available on the [Apple App Store](https://apps.apple.com) and [Google Play Store](https://play.google.com).

For full documentation visit [docs.servomote.app](https://docs.servomote.app).

## Sketches

There are three flavors to choose from depending on your hardware setup:

### 1. ESP32 Direct (`esp32/servomote-esp32/`)

Controls servos directly from ESP32 GPIO pins using the `ESP32Servo` library. This is the simplest setup — no additional hardware required beyond an ESP32 board and your servos.

- **Servo channels:** 3 (configurable)
- **Default pins:** GPIO 2, 3, 4
- **Library:** `ESP32Servo`

### 2. PCA9685 (`servomote-esp32-pca9685/`)

Uses a PCA9685 16-channel PWM driver over I2C for controlling up to 16 servos. Great for projects that need more channels or want to offload PWM generation.

- **Servo channels:** 16
- **Default I2C pins:** SDA = GPIO 6, SCL = GPIO 7
- **Library:** `Adafruit_PWMServoDriver`

### 3. M5Stack 8Servo Unit (`servomote-esp32-m5-8servo/`)

Uses the M5Stack 8-Channel Servo Driver Unit (STM32F030) over I2C for controlling up to 8 servos.

- **Servo channels:** 8
- **Default I2C pins:** SDA = GPIO 13, SCL = GPIO 15
- **Library:** `M5_UNIT_8SERVO`

## Getting Started

1. Install the [Arduino IDE](https://www.arduino.cc/en/software) and add ESP32 board support.
2. Pick the sketch that matches your hardware setup.
3. Open the `.ino` file in Arduino IDE.
4. Install the required libraries for your chosen sketch.
5. Flash the sketch to your ESP32.
6. Open ServoMote on your phone and connect to your ESP32.

## Customization

You are free to modify these sketches however you see fit. As long as the BLE API contract stays the same, the ServoMote app will continue to work. The API is straightforward:

- **BLE Service UUID:** `4fafc201-1fb5-459e-8fcc-c5c9c331914b`
- **BLE Characteristic UUID:** `beb5483e-36e1-4688-b7f5-ea07361b26a8`
- **Command format:** `servo_id:angle` (e.g., `1:90`)
- **Response format:** `OK:servo_id:angle` on success, `ERROR:message` on failure
- **Device name:** `ServoMote`

### Pin Configuration

For the two sketches with external servo controllers (PCA9685 and M5Stack 8Servo), make sure to update the I2C pin definitions (`SDA_PIN` and `SCL_PIN`) to match your specific ESP32 board. The defaults may not be correct for your setup.

For the direct ESP32 sketch, update the `SERVO_PINS` array to match the GPIO pins you have your servos connected to.
