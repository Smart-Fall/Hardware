/*
 * Simple I2C Bus Scanner - Diagnose I2C connectivity
 * Direct test of I2C on Feather V2 pins: SDA=GPIO22, SCL=GPIO20
 */

#include <Wire.h>

void setup() {
    Serial.begin(115200);
    delay(3000);

    Serial.println("\n\n=== I2C Bus Scanner ===");
    Serial.println("Testing I2C on: SDA=GPIO22, SCL=GPIO20");

    // Initialize I2C with Feather V2 pins
    Wire.begin(22, 20);
    Wire.setClock(100000);

    Serial.println("I2C initialized, starting scan...\n");
}

void loop() {
    byte error, address;
    int nDevices = 0;

    Serial.println("--- Scanning I2C bus ---");

    for (address = 1; address < 127; address++) {
        Wire.beginTransmission(address);
        error = Wire.endTransmission();

        if (error == 0) {
            Serial.print("Device at 0x");
            if (address < 16) Serial.print("0");
            Serial.print(address, HEX);
            Serial.print(" (decimal ");
            Serial.print(address);
            Serial.println(")");
            nDevices++;
        }
    }

    Serial.print("Total devices found: ");
    Serial.println(nDevices);

    if (nDevices == 0) {
        Serial.println("ERROR: No I2C devices!");
        Serial.println("Check wiring and pull-up resistors");
    } else {
        Serial.println("BMP280 addresses: 0x76 or 0x77");
    }

    Serial.println("");
    delay(5000);
}
