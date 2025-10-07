#ifndef MPU6050_SENSOR_H
#define MPU6050_SENSOR_H

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>

class MPU6050_Sensor {
private:
    Adafruit_MPU6050 mpu;
    bool initialized;
    uint8_t sda_pin;
    uint8_t scl_pin;

public:
    MPU6050_Sensor(uint8_t sda = 23, uint8_t scl = 22);

    bool begin();
    void configure(mpu6050_accel_range_t accel_range = MPU6050_RANGE_8_G,
                   mpu6050_gyro_range_t gyro_range = MPU6050_RANGE_1000_DEG,
                   mpu6050_bandwidth_t bandwidth = MPU6050_BAND_94_HZ);

    bool readData(float &accel_x, float &accel_y, float &accel_z,
                  float &gyro_x, float &gyro_y, float &gyro_z,
                  float &temp);

    bool isInitialized();
    void printInfo();
};

#endif
