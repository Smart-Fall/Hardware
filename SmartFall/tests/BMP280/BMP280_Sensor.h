#ifndef BMP280_SENSOR_H
#define BMP280_SENSOR_H

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_BMP280.h>

class BMP280_Sensor {
private:
    Adafruit_BMP280 bmp;
    bool initialized;
    uint8_t sda_pin;
    uint8_t scl_pin;
    float baselineAltitude;
    float seaLevelPressure;

    // Retry configuration for I2C communication
    static const uint8_t MAX_RETRIES = 10;
    static const uint16_t RETRY_DELAY_MS = 10;

    // Stale data detection
    float last_pressure = -999.0f;
    uint8_t stale_count = 0;
    static const uint8_t STALE_THRESHOLD = 3;  // 3 consecutive identical reads = stale

public:
    BMP280_Sensor(uint8_t sda = 255, uint8_t scl = 255);

    bool begin(uint8_t address = 0x76);
    void configure();
    void setSeaLevelPressure(float pressure_hPa);
    void resetBaselineAltitude();

    bool readData(float &temperature, float &pressure, float &altitude);
    float getAltitudeChange();

    bool isInitialized();
    void printInfo();
};

#endif
