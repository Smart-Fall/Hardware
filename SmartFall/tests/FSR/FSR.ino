/*
 * FSR (Force Sensitive Resistor) Sensor Test
 * ESP32 Feather V2
 *
 * Wiring:
 * FSR one end -> 3.3V
 * FSR other end -> A2 (GPIO 34) and 22kΩ pull-up resistor to 3.3V
 * FSR signal -> 0.1µF capacitor to GND for filtering
 */

#include "FSR_Sensor.h"

FSR_Sensor fsr(A2);

void setup() {
    Serial.begin(115200);
    delay(2000);

    Serial.println("\n=== FSR Sensor Test ===\n");

    if (!fsr.begin()) {
        Serial.println("ERROR: FSR initialization failed!");
        while (1) delay(1000);
    }

    Serial.println("✓ FSR initialized");

    fsr.printInfo();
    fsr.calibrate();

    Serial.println("\nPress the FSR sensor...\n");
}

void loop() {
    uint16_t raw = fsr.readRaw();
    float force = fsr.readForce();
    bool impact = fsr.detectImpact(500);

    Serial.println("--- FSR Data ---");
    Serial.print("Raw Value: ");
    Serial.println(raw);

    Serial.print("Force: ");
    Serial.print(force, 2);
    Serial.println(" N");

    if (impact) {
        Serial.println("⚠ IMPACT DETECTED!");
    }

    Serial.println();

    delay(200);
}
