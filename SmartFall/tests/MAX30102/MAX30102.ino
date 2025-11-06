/*
 * MAX30102 Sensor Test
 * ESP32 Feather V2
 *
 * Wiring:
 * MAX30102 VCC -> 3.3V
 * MAX30102 GND -> GND
 * MAX30102 SDA -> GPIO 22
 * MAX30102 SCL -> GPIO 20
 */

#include "MAX30102_Sensor.h"

MAX30102_Sensor heartSensor;

void setup() {
    Serial.begin(115200);
    delay(5000);

    Serial.println("\n=== MAX30102 Test ===\n");
    Serial.println("I2C Configuration:");
    Serial.println("  SDA: GPIO 22");
    Serial.println("  SCL: GPIO 20");
    Serial.println("  Expected Address: 0x57");
    Serial.println("");

    // Initialize I2C manually to test bus
    Wire.begin(22, 20);

    // Scan for MAX30102 at address 0x57
    Serial.println("Scanning I2C bus...");
    Wire.beginTransmission(0x57);
    byte error = Wire.endTransmission();

    if (error == 0) {
        Serial.println("✓ MAX30102 (0x57) found on I2C bus!");
    } else {
        Serial.println("ERROR: MAX30102 (0x57) NOT found on I2C bus!");
        Serial.print("I2C Error Code: ");
        Serial.println(error);
        Serial.println("\nDiagnostics:");
        Serial.println("- Check wiring (GPIO 20 = SCL, GPIO 22 = SDA)");
        Serial.println("- Check pull-up resistors");
        Serial.println("- Verify sensor is powered (3.3V + GND)");
        while (1) delay(1000);
    }

    Serial.println("\nAttempting sensor reset...");
    Serial.println("  Cycling power (if available)");
    Serial.println("  OR try the following:");
    Serial.println("  1. Disconnect sensor power for 5 seconds");
    Serial.println("  2. Reconnect sensor power");
    Serial.println("  3. Restart this sketch");
    delay(2000);

    Serial.println("\nInitializing MAX30102 driver...");
    if (!heartSensor.begin()) {
        Serial.println("ERROR: MAX30102 initialization failed!");
        Serial.println("\nSensor found on bus but failed to initialize");
        Serial.println("PART_ID returned 0xFF (all ones) - typical of:");
        Serial.println("  - Sensor in undefined state (needs hard reset)");
        Serial.println("  - Defective sensor");
        Serial.println("  - I2C bus timing issue");
        Serial.println("\nAction items:");
        Serial.println("  1. Power cycle the sensor (unplug 3.3V for 10 seconds)");
        Serial.println("  2. Check if sensor has a RST pin - toggle it LOW");
        Serial.println("  3. Try a different MAX30102 module if available");
        while (1) delay(1000);
    }

    Serial.println("✓ MAX30102 initialized");

    heartSensor.configure();
    heartSensor.printInfo();

    Serial.println("\nPlace finger on sensor...\n");
}

void loop() {
    float bpm;
    bool fingerDetected;

    if (heartSensor.readHeartRate(bpm, fingerDetected)) {
        Serial.println("--- MAX30102 Data ---");

        if (!fingerDetected) {
            Serial.println("No finger detected");
        } else {
            Serial.print("Heart Rate: ");
            Serial.print(bpm, 1);
            Serial.println(" BPM");

            Serial.print("IR Value: ");
            Serial.println(heartSensor.getIRValue());
        }
        Serial.println();
    }

    delay(100);
}
