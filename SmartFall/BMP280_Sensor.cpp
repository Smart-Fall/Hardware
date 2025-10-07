#include "sensors/BMP280_Sensor.h"

BMP280_Sensor::BMP280_Sensor(uint8_t sda, uint8_t scl)
    : initialized(false), sda_pin(sda), scl_pin(scl),
      baselineAltitude(0.0), seaLevelPressure(1013.25) {
}

bool BMP280_Sensor::begin(uint8_t address) {
    Wire.begin(sda_pin, scl_pin);

    if (!bmp.begin(address)) {
        // Try alternate address
        if (address == 0x76 && bmp.begin(0x77)) {
            initialized = true;
            return true;
        }
        Serial.println("Failed to initialize BMP280");
        return false;
    }

    initialized = true;
    return true;
}

void BMP280_Sensor::configure() {
    if (!initialized) return;

    bmp.setSampling(Adafruit_BMP280::MODE_NORMAL,
                    Adafruit_BMP280::SAMPLING_X2,
                    Adafruit_BMP280::SAMPLING_X16,
                    Adafruit_BMP280::FILTER_X16,
                    Adafruit_BMP280::STANDBY_MS_1);
}

void BMP280_Sensor::setSeaLevelPressure(float pressure_hPa) {
    seaLevelPressure = pressure_hPa;
}

void BMP280_Sensor::resetBaselineAltitude() {
    if (!initialized) return;

    baselineAltitude = bmp.readAltitude(seaLevelPressure);
    Serial.print("Baseline altitude set to: ");
    Serial.print(baselineAltitude, 2);
    Serial.println(" m");
}

bool BMP280_Sensor::readData(float &temperature, float &pressure, float &altitude) {
    if (!initialized) return false;

    temperature = bmp.readTemperature();
    pressure = bmp.readPressure() / 100.0;  // Pa to hPa
    altitude = bmp.readAltitude(seaLevelPressure);

    return true;
}

float BMP280_Sensor::getAltitudeChange() {
    if (!initialized) return 0.0;

    float current_altitude = bmp.readAltitude(seaLevelPressure);
    return current_altitude - baselineAltitude;
}

bool BMP280_Sensor::isInitialized() {
    return initialized;
}

void BMP280_Sensor::printInfo() {
    if (!initialized) {
        Serial.println("BMP280 not initialized");
        return;
    }

    Serial.println("=== BMP280 Info ===");
    Serial.println("Mode: NORMAL");
    Serial.println("Pressure oversampling: X16");
    Serial.println("Temperature oversampling: X2");
    Serial.println("Filter: X16");
}
