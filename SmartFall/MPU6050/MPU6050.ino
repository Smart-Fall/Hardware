/*
 * MPU6050 Sensor Test
 *
 * Wiring:
 * MPU6050 VCC -> 3.3V
 * MPU6050 GND -> GND
 * MPU6050 SDA -> GPIO 23
 * MPU6050 SCL -> GPIO 22
 */

#include "MPU6050_Sensor.h"

MPU6050_Sensor mpu;

void setup() {
    Serial.begin(115200);
    delay(2000);

    Serial.println("\n=== MPU6050 Test ===\n");

    if (!mpu.begin()) {
        Serial.println("ERROR: MPU6050 initialization failed!");
        while (1) delay(1000);
    }

    Serial.println("✓ MPU6050 initialized");

    mpu.configure(MPU6050_RANGE_8_G, MPU6050_RANGE_1000_DEG, MPU6050_BAND_94_HZ);
    mpu.printInfo();

    Serial.println("\nReading sensor data...\n");
}

void loop() {
    float ax, ay, az, gx, gy, gz, temp;

    if (mpu.readData(ax, ay, az, gx, gy, gz, temp)) {
        Serial.println("--- MPU6050 Data ---");
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
