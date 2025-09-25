#ifndef BMP280_SENSOR_H
#define BMP280_SENSOR_H

#include "../utils/data_types.h"
#include "../utils/config.h"
#include <Arduino.h>
#include <DHT.h>  // Using DHT22 for Wokwi simulation

class BMP280Sensor {
private:
    DHT dht;  // DHT22 for environmental sensor simulation
    bool initialized;
    float baseline_pressure;
    float baseline_altitude;
    uint32_t last_read_time;
    float altitude_history[10];  // Rolling average for stability
    uint8_t history_index;

public:
    BMP280Sensor();
    ~BMP280Sensor();

    // Initialization and configuration
    bool init();
    bool isInitialized() const;

    // Data reading functions
    float readPressure();
    float readTemperature();
    float readAltitude();
    float getAltitudeChange();

    // Calibration and baseline functions
    void resetAltitudeBaseline();
    void setSeaLevelPressure(float pressure_hpa);

    // Utility functions
    bool isDataReady();
    float pressureToAltitude(float pressure_hpa, float sea_level_hpa = 1013.25f);

    // Debug functions
    void printSensorInfo();
    void printRawData();
};

#endif // BMP280_SENSOR_H