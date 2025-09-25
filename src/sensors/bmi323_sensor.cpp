#include "bmi323_sensor.h"

BMI323Sensor::BMI323Sensor() : initialized(false), last_read_time(0) {
    // Initialize calibration arrays
    for(int i = 0; i < 3; i++) {
        accel_calibration[i] = 0.0f;
        gyro_calibration[i] = 0.0f;
    }
}

BMI323Sensor::~BMI323Sensor() {
    // Cleanup if needed
}

bool BMI323Sensor::init() {
    Wire.begin(MPU6050_SDA_PIN, MPU6050_SCL_PIN);

    if (!mpu.begin()) {
        Serial.println("Failed to initialize MPU6050 (BMI-323 simulation)");
        return false;
    }

    // Configure MPU6050 for fall detection
    mpu.setAccelerometerRange(MPU6050_RANGE_8_G);  // ±8g range for impact detection
    mpu.setGyroRange(MPU6050_RANGE_1000_DEG);      // ±1000°/s for rotation detection
    mpu.setFilterBandwidth(MPU6050_BAND_94_HZ);    // Anti-aliasing filter

    initialized = true;
    Serial.println("BMI-323 sensor (MPU6050) initialized successfully");

    return true;
}

void BMI323Sensor::setSampleRate(uint16_t rate_hz) {
    if (!initialized) return;

    // MPU6050 has fixed sample rate, but we can control reading frequency
    // This is handled by the main loop timing
    Serial.print("Sample rate set to: ");
    Serial.print(rate_hz);
    Serial.println(" Hz");
}

bool BMI323Sensor::isInitialized() const {
    return initialized;
}

bool BMI323Sensor::readAcceleration(float& x, float& y, float& z) {
    if (!initialized) return false;

    sensors_event_t accel;
    mpu.getAccelerometerSensor()->getEvent(&accel);

    // Apply calibration offsets
    x = (accel.acceleration.x / 9.81f) - accel_calibration[0];  // Convert to g
    y = (accel.acceleration.y / 9.81f) - accel_calibration[1];
    z = (accel.acceleration.z / 9.81f) - accel_calibration[2];

    return true;
}

bool BMI323Sensor::readAngularVelocity(float& x, float& y, float& z) {
    if (!initialized) return false;

    sensors_event_t gyro;
    mpu.getGyroSensor()->getEvent(&gyro);

    // Convert from rad/s to °/s and apply calibration
    x = (gyro.gyro.x * 180.0f / PI) - gyro_calibration[0];
    y = (gyro.gyro.y * 180.0f / PI) - gyro_calibration[1];
    z = (gyro.gyro.z * 180.0f / PI) - gyro_calibration[2];

    return true;
}

bool BMI323Sensor::readSensorData(SensorData_t& data) {
    if (!initialized) return false;

    bool accel_ok = readAcceleration(data.accel_x, data.accel_y, data.accel_z);
    bool gyro_ok = readAngularVelocity(data.gyro_x, data.gyro_y, data.gyro_z);

    data.timestamp = millis();
    data.valid = accel_ok && gyro_ok;
    last_read_time = data.timestamp;

    return data.valid;
}

float BMI323Sensor::getTotalAcceleration() {
    float x, y, z;
    if (readAcceleration(x, y, z)) {
        return sqrt(x*x + y*y + z*z);
    }
    return 0.0f;
}

float BMI323Sensor::getAngularMagnitude() {
    float x, y, z;
    if (readAngularVelocity(x, y, z)) {
        return sqrt(x*x + y*y + z*z);
    }
    return 0.0f;
}

bool BMI323Sensor::isDataReady() {
    if (!initialized) return false;

    // For MPU6050, data is always ready when requested
    // In real BMI-323, this would check a status register
    return (millis() - last_read_time) >= (1000 / SENSOR_SAMPLE_RATE_HZ);
}

void BMI323Sensor::calibrateSensor() {
    if (!initialized) return;

    Serial.println("Calibrating BMI-323 sensor... Keep device still!");

    const int samples = 100;
    float accel_sum[3] = {0, 0, 0};
    float gyro_sum[3] = {0, 0, 0};

    for (int i = 0; i < samples; i++) {
        float ax, ay, az, gx, gy, gz;

        if (readAcceleration(ax, ay, az)) {
            accel_sum[0] += ax;
            accel_sum[1] += ay;
            accel_sum[2] += az - 1.0f;  // Subtract 1g for Z-axis gravity
        }

        if (readAngularVelocity(gx, gy, gz)) {
            gyro_sum[0] += gx;
            gyro_sum[1] += gy;
            gyro_sum[2] += gz;
        }

        delay(10);
    }

    // Calculate average offsets
    for (int i = 0; i < 3; i++) {
        accel_calibration[i] = accel_sum[i] / samples;
        gyro_calibration[i] = gyro_sum[i] / samples;
    }

    Serial.println("Calibration complete!");
    Serial.print("Accel offsets: ");
    Serial.print(accel_calibration[0]); Serial.print(", ");
    Serial.print(accel_calibration[1]); Serial.print(", ");
    Serial.println(accel_calibration[2]);

    Serial.print("Gyro offsets: ");
    Serial.print(gyro_calibration[0]); Serial.print(", ");
    Serial.print(gyro_calibration[1]); Serial.print(", ");
    Serial.println(gyro_calibration[2]);
}

void BMI323Sensor::resetCalibration() {
    for(int i = 0; i < 3; i++) {
        accel_calibration[i] = 0.0f;
        gyro_calibration[i] = 0.0f;
    }
    Serial.println("Sensor calibration reset");
}

void BMI323Sensor::enterSleepMode() {
    // MPU6050 doesn't have the same sleep modes as BMI-323
    // This is a placeholder for power management
    Serial.println("BMI-323 entering sleep mode");
}

void BMI323Sensor::wakeUp() {
    Serial.println("BMI-323 waking up");
}

void BMI323Sensor::printSensorInfo() {
    if (!initialized) {
        Serial.println("BMI-323 sensor not initialized");
        return;
    }

    Serial.println("=== BMI-323 Sensor Info ===");
    Serial.println("Sensor: MPU6050 (Wokwi simulation)");
    Serial.println("Accelerometer Range: ±8g");
    Serial.println("Gyroscope Range: ±1000°/s");
    Serial.print("Sample Rate: ");
    Serial.print(SENSOR_SAMPLE_RATE_HZ);
    Serial.println(" Hz");
    Serial.println("===========================");
}

void BMI323Sensor::printRawData() {
    if (!initialized) return;

    float ax, ay, az, gx, gy, gz;
    if (readAcceleration(ax, ay, az) && readAngularVelocity(gx, gy, gz)) {
        Serial.print("Accel(g): ");
        Serial.print(ax, 3); Serial.print(", ");
        Serial.print(ay, 3); Serial.print(", ");
        Serial.print(az, 3);

        Serial.print(" | Gyro(°/s): ");
        Serial.print(gx, 2); Serial.print(", ");
        Serial.print(gy, 2); Serial.print(", ");
        Serial.println(gz, 2);
    }
}