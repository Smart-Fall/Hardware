/*
 * BMP280 Sensor Test
 *
 * Wiring:
 * BMP280 VCC -> 3.3V
 * BMP280 GND -> GND
 * BMP280 SDA -> GPIO 23
 * BMP280 SCL -> GPIO 22
 */

#include "BMP280_Sensor.h"

BMP280_Sensor bmp;

void setup() {
    Serial.begin(115200);
    delay(2000);

    Serial.println("\n=== BMP280 Test ===\n");

    if (!bmp.begin()) {
        Serial.println("ERROR: BMP280 initialization failed!");
        Serial.println("Trying alternate address...");
        if (!bmp.begin(0x77)) {
            Serial.println("ERROR: BMP280 not found at 0x76 or 0x77");
            while (1) delay(1000);
        }
    }

    Serial.println("✓ BMP280 initialized");

    bmp.configure();
    bmp.printInfo();

    Serial.println("\nCalibrating baseline...");
    delay(2000);
    bmp.resetBaselineAltitude();

    Serial.println("\nReading sensor data...\n");
}

void loop() {
    float temp, pressure, altitude;

    if (bmp.readData(temp, pressure, altitude)) {
        Serial.println("--- BMP280 Data ---");
        Serial.print("Temperature: ");
        Serial.print(temp, 2);
        Serial.println(" °C");

        Serial.print("Pressure: ");
        Serial.print(pressure, 2);
        Serial.println(" hPa");

        Serial.print("Altitude: ");
        Serial.print(altitude, 2);
        Serial.println(" m");

        float altChange = bmp.getAltitudeChange();
        Serial.print("Altitude Change: ");
        Serial.print(altChange, 2);
        Serial.print(" m");

        if (abs(altChange) > 0.5) {
            Serial.print("  ⚠ SIGNIFICANT!");
        }
        Serial.println("\n");
    }

    delay(1000);
}
