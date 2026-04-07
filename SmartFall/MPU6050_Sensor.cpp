#include "MPU6050_Sensor.h"
#include "Board_Config.h"

MPU6050_Sensor::MPU6050_Sensor(uint8_t sda, uint8_t scl)
    : initialized(false),
      sda_pin(sda == 255 ? Board_Config::getSDA() : sda),
      scl_pin(scl == 255 ? Board_Config::getSCL() : scl),
      gyro_offset_x(0), gyro_offset_y(0), gyro_offset_z(0),
      accel_offset_x(0), accel_offset_y(0), accel_offset_z(0),
      calibrated(false)
{
}

bool MPU6050_Sensor::begin()
{
    // Wire.begin() is called once globally in initializeSensors().
    // Re-calling it here would reset the I2C bus and break all other sensors.
    for (uint8_t attempt = 0; attempt < MAX_RETRIES; attempt++)
    {
        if (mpu.begin())
        {
            initialized = true;
            return true;
        }
        if (attempt < MAX_RETRIES - 1)
        {
            Serial.printf("[MPU6050] Init retry %d/%d\n", attempt + 1, MAX_RETRIES);
            delay(RETRY_DELAY_MS);
        }
    }

    Serial.println("[MPU6050] Failed to initialize - all retries failed");
    return false;
}

void MPU6050_Sensor::configure(mpu6050_accel_range_t accel_range,
                               mpu6050_gyro_range_t gyro_range,
                               mpu6050_bandwidth_t bandwidth)
{
    if (!initialized)
        return;

    mpu.setAccelerometerRange(accel_range);
    mpu.setGyroRange(gyro_range);
    mpu.setFilterBandwidth(bandwidth);
}

bool MPU6050_Sensor::readData(float &accel_x, float &accel_y, float &accel_z,
                              float &gyro_x, float &gyro_y, float &gyro_z,
                              float &temp)
{
    if (!initialized)
        return false;

    // Probe the I2C bus before calling into the Adafruit library.
    // If the device doesn't ACK (loose wiring), mpu.getEvent() would crash
    // the ESP32 with a StoreProhibited panic. Returning false here lets the
    // higher-level recovery in readSensors() handle the reinit safely.
    Wire.beginTransmission(MPU6050_I2C_ADDRESS);
    if (Wire.endTransmission() != 0)
    {
        initialized = false;
        stale_count = 0;
        return false;
    }

    sensors_event_t a, g, t;
    mpu.getEvent(&a, &g, &t);

    accel_x = a.acceleration.x / 9.81; // Convert to g
    accel_y = a.acceleration.y / 9.81;
    accel_z = a.acceleration.z / 9.81;

    // Adafruit library returns rad/s; convert to °/s to match thresholds
    gyro_x = g.gyro.x * (180.0f / M_PI);
    gyro_y = g.gyro.y * (180.0f / M_PI);
    gyro_z = g.gyro.z * (180.0f / M_PI);

    temp = t.temperature;

    // Check for stale data (same values repeatedly = sensor frozen)
    const float stale_epsilon = 0.002f;
    if (fabsf(accel_x - last_accel_x) < stale_epsilon &&
        fabsf(accel_y - last_accel_y) < stale_epsilon &&
        fabsf(accel_z - last_accel_z) < stale_epsilon)
    {
        stale_count++;
        if (stale_count >= STALE_THRESHOLD)
        {
            Serial.printf("[MPU6050] Stale data detected after %d identical reads\n", STALE_THRESHOLD);
            stale_count = 0;
            // Mark uninitialized — outer recovery in readSensors() handles reinit
            initialized = false;
            return false;
        }
    }
    else
    {
        stale_count = 0;
        last_accel_x = accel_x;
        last_accel_y = accel_y;
        last_accel_z = accel_z;
    }

    // Apply calibration offsets if calibrated
    if (calibrated)
    {
        accel_x -= accel_offset_x;
        accel_y -= accel_offset_y;
        accel_z -= accel_offset_z;

        gyro_x -= gyro_offset_x;
        gyro_y -= gyro_offset_y;
        gyro_z -= gyro_offset_z;
    }

    return true;
}

void MPU6050_Sensor::calibrate(uint16_t samples)
{
    if (!initialized)
    {
        Serial.println("MPU6050 not initialized. Cannot calibrate.");
        return;
    }

    Serial.println("Calibrating MPU6050 - keep sensor stationary...");
    delay(2000); // Give user time to stabilize sensor

    float sum_gyro_x = 0, sum_gyro_y = 0, sum_gyro_z = 0;
    float sum_accel_x = 0, sum_accel_y = 0, sum_accel_z = 0;

    Serial.print("Taking ");
    Serial.print(samples);
    Serial.println(" samples...");

    for (uint16_t i = 0; i < samples; i++)
    {
        sensors_event_t a, g, t;
        mpu.getEvent(&a, &g, &t);

        sum_accel_x += a.acceleration.x / 9.81;
        sum_accel_y += a.acceleration.y / 9.81;
        sum_accel_z += a.acceleration.z / 9.81;

        // Accumulate in °/s to match readData() output units
        sum_gyro_x += g.gyro.x * (180.0f / M_PI);
        sum_gyro_y += g.gyro.y * (180.0f / M_PI);
        sum_gyro_z += g.gyro.z * (180.0f / M_PI);

        delay(5); // Small delay between samples
    }

    // Calculate average offsets
    gyro_offset_x = sum_gyro_x / samples;
    gyro_offset_y = sum_gyro_y / samples;
    gyro_offset_z = sum_gyro_z / samples;

    accel_offset_x = sum_accel_x / samples;
    accel_offset_y = sum_accel_y / samples;
    // For Z-axis, we expect 1g when stationary (gravity), so offset from 1g
    accel_offset_z = (sum_accel_z / samples) - 1.0;

    calibrated = true;

    Serial.println("MPU6050 Calibration complete!");
    Serial.print("Gyro offsets (°/s): X=");
    Serial.print(gyro_offset_x, 4);
    Serial.print(", Y=");
    Serial.print(gyro_offset_y, 4);
    Serial.print(", Z=");
    Serial.println(gyro_offset_z, 4);

    Serial.print("Accel offsets (g): X=");
    Serial.print(accel_offset_x, 4);
    Serial.print(", Y=");
    Serial.print(accel_offset_y, 4);
    Serial.print(", Z=");
    Serial.println(accel_offset_z, 4);
}

bool MPU6050_Sensor::isCalibrated()
{
    return calibrated;
}

bool MPU6050_Sensor::isInitialized()
{
    return initialized;
}

void MPU6050_Sensor::printInfo()
{
    if (!initialized)
    {
        Serial.println("MPU6050 not initialized");
        return;
    }

    Serial.println("=== MPU6050 Info ===");
    Serial.print("Accelerometer range: ±");
    switch (mpu.getAccelerometerRange())
    {
    case MPU6050_RANGE_2_G:
        Serial.println("2G");
        break;
    case MPU6050_RANGE_4_G:
        Serial.println("4G");
        break;
    case MPU6050_RANGE_8_G:
        Serial.println("8G");
        break;
    case MPU6050_RANGE_16_G:
        Serial.println("16G");
        break;
    }

    Serial.print("Gyroscope range: ±");
    switch (mpu.getGyroRange())
    {
    case MPU6050_RANGE_250_DEG:
        Serial.println("250°/s");
        break;
    case MPU6050_RANGE_500_DEG:
        Serial.println("500°/s");
        break;
    case MPU6050_RANGE_1000_DEG:
        Serial.println("1000°/s");
        break;
    case MPU6050_RANGE_2000_DEG:
        Serial.println("2000°/s");
        break;
    }
}
