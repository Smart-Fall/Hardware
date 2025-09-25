#ifndef BMI323_SENSOR_H
#define BMI323_SENSOR_H

#include "../utils/data_types.h"
#include "../utils/config.h"
#include <Arduino.h>
#include <Adafruit_MPU6050.h> // Using MPU6050 for Wokwi simulation
#include <Adafruit_Sensor.h>
#include <Wire.h>

class BMI323Sensor
{
private:
    Adafruit_MPU6050 mpu; // MPU6050 for simulation (functionally equivalent to BMI-323)
    bool initialized;
    float accel_calibration[3]; // Calibration offsets
    float gyro_calibration[3];
    uint32_t last_read_time;

public:
    BMI323Sensor();
    ~BMI323Sensor();

    // Initialization and configuration
    bool init();
    void setSampleRate(uint16_t rate_hz);
    bool isInitialized() const;

    // Data reading functions
    bool readAcceleration(float &x, float &y, float &z);
    bool readAngularVelocity(float &x, float &y, float &z);
    bool readSensorData(SensorData_t &data);

    // Utility functions
    float getTotalAcceleration();
    float getAngularMagnitude();
    bool isDataReady();

    // Calibration functions
    void calibrateSensor();
    void resetCalibration();

    // Power management
    void enterSleepMode();
    void wakeUp();

    // Debug functions
    void printSensorInfo();
    void printRawData();
};

#endif // BMI323_SENSOR_H