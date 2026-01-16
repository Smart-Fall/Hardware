/*
 * MPU6050Sensor + BMP280Sensor Combined Test Sketch
 *
 * Test program using modular MPU6050_Sensor and BMP280_Sensor classes
 *
 * Hardware: ESP32 Feather V2
 * Sensors: MPU6050Sensor (Accelerometer + Gyroscope) + BMP280Sensor (Pressure + Temperature)
 *
 * Wiring (I2C shared bus):
 * MPU6050Sensor VCC -> 3.3V
 * MPU6050Sensor GND -> GND
 * MPU6050Sensor SDA -> GPIO 22
 * MPU6050Sensor SCL -> GPIO 20
 *
 * BMP280Sensor VCC -> 3.3V
 * BMP280Sensor GND -> GND
 * BMP280Sensor SDA -> GPIO 22 (shared)
 * BMP280Sensor SCL -> GPIO 20 (shared)
 */

#include "MPU6050_Sensor.h"
#include "BMP280_Sensor.h"

// Pin definitions for ESP32 HUZZAH32
#define SDA_PIN 23
#define SCL_PIN 22

// Create sensor objects
MPU6050_Sensor imuSensor(SDA_PIN, SCL_PIN);
BMP280_Sensor pressureSensor(SDA_PIN, SCL_PIN);

void setup() {
  // Initialize serial communication
  Serial.begin(115200);
  delay(2000);  // Wait for serial monitor

  Serial.println("\n=== MPU6050Sensor + BMP280Sensor Combined Test ===\n");

  // Initialize MPU6050
  Serial.println("--- Initializing MPU6050Sensor ---");
  if (!imuSensor.begin()) {
    Serial.println("✗ ERROR: Failed to initialize MPU6050!");
    Serial.println("  Check wiring and I2C address (default: 0x68)");
  } else {
    Serial.println("✓ MPU6050Sensor initialized successfully!");

    // Configure sensor ranges
    imuSensor.configure(MPU6050_RANGE_8_G,
                        MPU6050_RANGE_1000_DEG,
                        MPU6050_BAND_94_HZ);

    imuSensor.printInfo();
  }

  // Initialize BMP280
  Serial.println("\n--- Initializing BMP280Sensor ---");

  // Try default address 0x76, then alternate 0x77
  if (!pressureSensor.begin(0x76)) {
    if (!pressureSensor.begin(0x77)) {
      Serial.println("✗ ERROR: Failed to initialize BMP280!");
      Serial.println("  Tried addresses: 0x76, 0x77");
    } else {
      Serial.println("✓ BMP280Sensor found at address 0x77!");
      pressureSensor.configure();
    }
  } else {
    Serial.println("✓ BMP280Sensor found at address 0x76!");
    pressureSensor.configure();
  }

  // Set baseline altitude
  pressureSensor.resetBaselineAltitude();

  Serial.println("\n=== Sensors Ready ===");
  Serial.println("Reading sensor data every 1 second...\n");

  delay(1000);
}

void loop() {
  // Print timestamp
  Serial.println("=====================================");
  Serial.print("Time: ");
  Serial.print(millis() / 1000.0, 2);
  Serial.println(" s\n");

  // Read MPU6050Sensor data
  readMPU6050Data();

  Serial.println();

  // Read BMP280Sensor data
  readBMP280Data();

  Serial.println("=====================================\n");

  delay(1000);  // Read every 1 second
}

void readMPU6050Data() {
  float ax, ay, az;      // Acceleration (m/s²)
  float gx, gy, gz;      // Gyroscope (rad/s)
  float temp;            // Temperature (°C)

  if (!imuSensor.readData(ax, ay, az, gx, gy, gz, temp)) {
    Serial.println("MPU6050: Failed to read data");
    return;
  }

  Serial.println("--- MPU6050Sensor Data ---");

  // Acceleration (convert m/s² to g)
  Serial.println("Acceleration:");
  Serial.print("  X: ");
  Serial.print(ax / 9.81, 2);
  Serial.print(" g  (");
  Serial.print(ax, 2);
  Serial.println(" m/s²)");

  Serial.print("  Y: ");
  Serial.print(ay / 9.81, 2);
  Serial.print(" g  (");
  Serial.print(ay, 2);
  Serial.println(" m/s²)");

  Serial.print("  Z: ");
  Serial.print(az / 9.81, 2);
  Serial.print(" g  (");
  Serial.print(az, 2);
  Serial.println(" m/s²)");

  // Total acceleration magnitude
  float total_accel = sqrt(ax * ax + ay * ay + az * az) / 9.81;
  Serial.print("  Total: ");
  Serial.print(total_accel, 2);
  Serial.println(" g");

  // Rotation (convert rad/s to deg/s)
  Serial.println("\nRotation:");
  Serial.print("  X: ");
  Serial.print(gx * 180.0 / PI, 2);
  Serial.println(" °/s");

  Serial.print("  Y: ");
  Serial.print(gy * 180.0 / PI, 2);
  Serial.println(" °/s");

  Serial.print("  Z: ");
  Serial.print(gz * 180.0 / PI, 2);
  Serial.println(" °/s");

  // Angular magnitude
  float angular_mag = sqrt(gx * gx + gy * gy + gz * gz) * 180.0 / PI;
  Serial.print("  Magnitude: ");
  Serial.print(angular_mag, 2);
  Serial.println(" °/s");

  // Temperature
  Serial.print("\nTemperature: ");
  Serial.print(temp, 1);
  Serial.println(" °C");
}

void readBMP280Data() {
  float temperature;
  float pressure;
  float altitude;

  if (!pressureSensor.readData(temperature, pressure, altitude)) {
    Serial.println("BMP280: Failed to read data");
    return;
  }

  Serial.println("--- BMP280Sensor Data ---");

  Serial.print("Pressure: ");
  Serial.print(pressure, 2);
  Serial.println(" hPa");

  Serial.println();
}
