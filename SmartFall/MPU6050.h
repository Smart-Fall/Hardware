#ifndef MPU6050_H
#define MPU6050_H

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>

class MPU6050Sensor
{
private:
    Adafruit_MPU6050 mpu;
    bool initialized;
    uint8_t sda_pin;
    uint8_t scl_pin;

    // Calibration offsets
    float gyro_offset_x;
    float gyro_offset_y;
    float gyro_offset_z;
    float accel_offset_x;
    float accel_offset_y;
    float accel_offset_z;
    bool calibrated;

public:
    MPU6050Sensor(uint8_t sda = 22, uint8_t scl = 20);

    bool begin();
    void configure(mpu6050_accel_range_t accel_range = MPU6050_RANGE_8_G,
                   mpu6050_gyro_range_t gyro_range = MPU6050_RANGE_1000_DEG,
                   mpu6050_bandwidth_t bandwidth = MPU6050_BAND_94_HZ);

    bool readData(float &accel_x, float &accel_y, float &accel_z,
                  float &gyro_x, float &gyro_y, float &gyro_z,
                  float &temp);

    void calibrate(uint16_t samples = 100);
    bool isCalibrated();
    bool isInitialized();
    void printInfo();
};

#endif
