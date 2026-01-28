/*
 * MPU6050Sensor Sensor Test
 * ESP32 Feather V2 / ESP32 HUZZAH32 Feather
 *
 * Wiring:
 * MPU6050Sensor VCC -> 3.3V
 * MPU6050Sensor GND -> GND
 * MPU6050Sensor SDA -> Auto-detected based on board
 * MPU6050Sensor SCL -> Auto-detected based on board
 */

#include "Board_Config.h"
#include "MPU6050_Sensor.h"

MPU6050_Sensor mpu;

void setup() {
    Serial.begin(115200);
    delay(2000);

    Serial.println("\n=== MPU6050Sensor Test ===\n");

    // Initialize board detection
    Board_Config::begin();

    if (!mpu.begin()) {
        Serial.println("ERROR: MPU6050Sensor initialization failed!");
        while (1) delay(1000);
    }

    Serial.println("✓ MPU6050Sensor initialized");

    mpu.configure(MPU6050_RANGE_8_G, MPU6050_RANGE_1000_DEG, MPU6050_BAND_94_HZ);
    mpu.printInfo();

    Serial.println("\nReading sensor data...\n");
}

void loop() {
    float ax, ay, az, gx, gy, gz, temp;

    if (mpu.readData(ax, ay, az, gx, gy, gz, temp)) {
        Serial.println("--- MPU6050Sensor Data ---");
        Serial.print("Accel (g): X=");
        Serial.print(ax, 2);
        Serial.print(" Y=");
        Serial.print(ay, 2);
        Serial.print(" Z=");
        Serial.println(az, 2);

        Serial.print("Gyro (°/s): X=");
        Serial.print(gx, 2);
        Serial.print(" Y=");
        Serial.print(gy, 2);
        Serial.print(" Z=");
        Serial.println(gz, 2);

        Serial.print("Temp: ");
        Serial.print(temp, 1);
        Serial.println(" °C\n");
    }

    delay(500);
}
