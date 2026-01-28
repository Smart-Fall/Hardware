/*
 * Board Detection Test
 *
 * This sketch verifies that the Board_Config utility correctly detects
 * the ESP32 board type and configures appropriate I2C pins.
 *
 * Expected Results:
 * - ESP32 HUZZAH32 Feather: SDA=GPIO 23, SCL=GPIO 22
 * - ESP32 Feather V2: SDA=GPIO 22, SCL=GPIO 20
 *
 * Upload this sketch to verify correct board detection before running
 * other sensor tests.
 */

#include "../../utils/Board_Config.h"
#include <Wire.h>

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    delay(10);
  }

  delay(1000);
  Serial.println("\n\n========================================");
  Serial.println("    Board Detection Test");
  Serial.println("========================================\n");

  // Initialize board detection
  Board_Config::begin();

  // Display detected configuration
  Serial.println("\n--- Detected Configuration ---");
  Serial.printf("Board Type: %d\n", Board_Config::getBoardType());
  Serial.printf("Board Name: %s\n", Board_Config::getBoardName());
  Serial.printf("SDA Pin: GPIO %d\n", Board_Config::getSDA());
  Serial.printf("SCL Pin: GPIO %d\n", Board_Config::getSCL());
  Serial.println("------------------------------\n");

  // Initialize I2C with detected pins
  Wire.begin(Board_Config::getSDA(), Board_Config::getSCL());
  Serial.println("I2C bus initialized with detected pins");

  // Scan for I2C devices
  Serial.println("\nScanning I2C bus for devices...");
  scanI2C();

  Serial.println("\n========================================");
  Serial.println("Board detection test complete!");
  Serial.println("If the configuration above matches your");
  Serial.println("board, you can proceed with sensor tests.");
  Serial.println("========================================\n");
}

void loop() {
  // Nothing to do in loop
  delay(5000);
  Serial.println("Board detection active. Configuration printed above.");
}

void scanI2C() {
  byte error, address;
  int deviceCount = 0;

  Serial.println("Scanning I2C addresses 0x01 to 0x7F...");

  for (address = 1; address < 127; address++) {
    Wire.beginTransmission(address);
    error = Wire.endTransmission();

    if (error == 0) {
      Serial.printf("  Device found at address 0x%02X\n", address);
      deviceCount++;
    }
  }

  if (deviceCount == 0) {
    Serial.println("  No I2C devices found");
    Serial.println("  (This is normal if no sensors are connected)");
  } else {
    Serial.printf("\nFound %d I2C device(s)\n", deviceCount);
  }
}
