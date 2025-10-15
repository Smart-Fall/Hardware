#include "sensors/MPU6050_Sensor.h"

MPU6050_Sensor::MPU6050_Sensor(uint8_t sda, uint8_t scl)
    : initialized(false), sda_pin(sda), scl_pin(scl) {
}

bool MPU6050_Sensor::begin() {
    Wire.begin(sda_pin, scl_pin);

    if (!mpu.begin()) {
        Serial.println("Failed to initialize MPU6050");
        return false;
    }

    initialized = true;
    return true;
}

void MPU6050_Sensor::configure(mpu6050_accel_range_t accel_range,
                                mpu6050_gyro_range_t gyro_range,
                                mpu6050_bandwidth_t bandwidth) {
    if (!initialized) return;

    mpu.setAccelerometerRange(accel_range);
    mpu.setGyroRange(gyro_range);
    mpu.setFilterBandwidth(bandwidth);
}

bool MPU6050_Sensor::readData(float &accel_x, float &accel_y, float &accel_z,
                               float &gyro_x, float &gyro_y, float &gyro_z,
                               float &temp) {
    if (!initialized) return false;

    sensors_event_t a, g, t;
    mpu.getEvent(&a, &g, &t);

    accel_x = a.acceleration.x / 9.81;  // Convert to g
    accel_y = a.acceleration.y / 9.81;
    accel_z = a.acceleration.z / 9.81;

    gyro_x = g.gyro.x;
    gyro_y = g.gyro.y;
    gyro_z = g.gyro.z;

    temp = t.temperature;

    return true;
}

bool MPU6050_Sensor::isInitialized() {
    return initialized;
}

void MPU6050_Sensor::printInfo() {
    if (!initialized) {
        Serial.println("MPU6050 not initialized");
        return;
    }

    Serial.println("=== MPU6050 Info ===");
    Serial.print("Accelerometer range: ±");
    switch (mpu.getAccelerometerRange()) {
        case MPU6050_RANGE_2_G: Serial.println("2G"); break;
        case MPU6050_RANGE_4_G: Serial.println("4G"); break;
        case MPU6050_RANGE_8_G: Serial.println("8G"); break;
        case MPU6050_RANGE_16_G: Serial.println("16G"); break;
    }

    Serial.print("Gyroscope range: ±");
    switch (mpu.getGyroRange()) {
        case MPU6050_RANGE_250_DEG: Serial.println("250°/s"); break;
        case MPU6050_RANGE_500_DEG: Serial.println("500°/s"); break;
        case MPU6050_RANGE_1000_DEG: Serial.println("1000°/s"); break;
        case MPU6050_RANGE_2000_DEG: Serial.println("2000°/s"); break;
    }
}
