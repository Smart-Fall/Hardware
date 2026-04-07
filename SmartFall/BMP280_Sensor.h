#ifndef BMP280_SENSOR_H
#define BMP280_SENSOR_H

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_BMP280.h>
#include "Config.h"

class BMP280_Sensor
{
private:
    Adafruit_BMP280 bmp{&Wire};
    bool initialized;
    uint8_t i2c_address;
    uint8_t sda_pin;
    uint8_t scl_pin;
    float baselineAltitude;
    float seaLevelPressure;

    // Retry configuration for I2C communication
    static const uint8_t MAX_RETRIES = I2C_SENSOR_MAX_RETRIES;
    static const uint16_t RETRY_DELAY_MS = I2C_SENSOR_RETRY_DELAY_MS;

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
