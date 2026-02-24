/*
 * BMP280Sensor Sensor Test
 * ESP32 Feather V2 / ESP32 HUZZAH32 Feather
 *
 * Wiring:
 * BMP280Sensor VCC -> 3.3V
 * BMP280Sensor GND -> GND
 * BMP280Sensor SDA -> Auto-detected based on board
 * BMP280Sensor SCL -> Auto-detected based on board
 */

#include "Board_Config.h"
#include "BMP280_Sensor.h"

BMP280_Sensor bmp;

void setup() {
    Serial.begin(115200);
    delay(2000);

    Serial.println("\n=== BMP280Sensor Test ===\n");

    // Initialize board detection
    Board_Config::begin();

    if (!bmp.begin()) {
        Serial.println("ERROR: BMP280Sensor initialization failed!");
        Serial.println("Trying alternate address...");
        if (!bmp.begin(0x77)) {
            Serial.println("ERROR: BMP280Sensor not found at 0x76 or 0x77");
            while (1) delay(1000);
        }
    }

    Serial.println("✓ BMP280Sensor initialized");

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
        Serial.println("--- BMP280Sensor Data ---");
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
