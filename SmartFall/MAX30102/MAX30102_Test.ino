/*
 * MAX30102 Sensor Test
 *
 * Wiring:
 * MAX30102 VCC -> 3.3V
 * MAX30102 GND -> GND
 * MAX30102 SDA -> GPIO 23
 * MAX30102 SCL -> GPIO 22
 */

#include "MAX30102_Sensor.h"

MAX30102_Sensor heartSensor;

void setup() {
    Serial.begin(115200);
    delay(2000);

    Serial.println("\n=== MAX30102 Test ===\n");

    if (!heartSensor.begin()) {
        Serial.println("ERROR: MAX30102 initialization failed!");
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
