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

public:
    BMP280_Sensor(uint8_t sda = 23, uint8_t scl = 22);

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
