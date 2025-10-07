/*
 * SmartFall - Wearable Fall Detection System
 *
 * Hardware: ESP32 HUZZAH32 Feather
 * Sensors: MPU6050 (IMU), BMP280 (Pressure), MAX30102 (Heart Rate), FSR (Force)
 *
 * This Arduino sketch provides real-time fall detection using multi-sensor fusion
 * and a confidence-based scoring system.
 */

#include "sensors/MPU6050_Sensor.h"
#include "sensors/BMP280_Sensor.h"
#include "sensors/MAX30102_Sensor.h"
#include "sensors/FSR_Sensor.h"
#include "detection/fall_detector.h"
#include "detection/confidence_scorer.h"
#include "utils/config.h"
#include "utils/data_types.h"

// Sensor instances
MPU6050_Sensor imuSensor;
BMP280_Sensor pressureSensor;
MAX30102_Sensor heartRateSensor;
FSR_Sensor forceSensor(FSR_ANALOG_PIN);

// Detection system
FallDetector fallDetector;
ConfidenceScorer confidenceScorer;

// System state
SensorData_t currentSensorData;
uint32_t lastSensorRead = 0;
bool systemInitialized = false;

void setup() {
  // Initialize serial communication
  Serial.begin(SERIAL_BAUD_RATE);
  delay(2000);  // Wait for serial monitor

  Serial.println("\n=== SmartFall Initialization ===");

  // Initialize SOS button
  pinMode(SOS_BUTTON_PIN, INPUT_PULLUP);

  // Initialize alert outputs
  pinMode(SPEAKER_PIN, OUTPUT);
  pinMode(HAPTIC_PIN, OUTPUT);
  pinMode(VISUAL_ALERT_PIN, OUTPUT);

  digitalWrite(SPEAKER_PIN, LOW);
  digitalWrite(HAPTIC_PIN, LOW);
  digitalWrite(VISUAL_ALERT_PIN, LOW);

  // Initialize sensors
  Serial.println("\n--- Initializing Sensors ---");

  if (!imuSensor.begin()) {
    Serial.println("ERROR: Failed to initialize MPU6050!");
  } else {
    Serial.println("✓ MPU6050 initialized");
    imuSensor.configure();
  }

  if (!pressureSensor.begin()) {
    Serial.println("ERROR: Failed to initialize BMP280!");
  } else {
    Serial.println("✓ BMP280 initialized");
    pressureSensor.configure();
    delay(1000);
    pressureSensor.resetBaselineAltitude();
  }

  if (!heartRateSensor.begin()) {
    Serial.println("WARNING: Failed to initialize MAX30102 (optional)");
  } else {
    Serial.println("✓ MAX30102 initialized");
    heartRateSensor.configure();
  }

  if (!forceSensor.begin()) {
    Serial.println("WARNING: Failed to initialize FSR (optional)");
  } else {
    Serial.println("✓ FSR initialized");
    forceSensor.calibrate();
  }

  // Initialize fall detector
  if (fallDetector.init()) {
    Serial.println("✓ Fall detector initialized");
    fallDetector.enableMonitoring();
  } else {
    Serial.println("ERROR: Failed to initialize fall detector!");
  }

  systemInitialized = true;
  Serial.println("\n=== SmartFall Ready ===");
  Serial.println("Monitoring for falls...\n");
}

void loop() {
  uint32_t currentTime = millis();

  // Check SOS button
  if (digitalRead(SOS_BUTTON_PIN) == LOW) {
    handleSOSButton();
  }

  // Read sensors at configured rate
  if (currentTime - lastSensorRead >= SENSOR_READ_INTERVAL_MS) {
    lastSensorRead = currentTime;

    // Read all sensors
    readSensors();

    // Process sensor data through fall detector
    fallDetector.processSensorData(currentSensorData);

    // Check fall status
    FallStatus_t status = fallDetector.getCurrentStatus();

    if (status == FALL_STATUS_FALL_DETECTED) {
      handleFallDetected();
    }

    // Debug output (optional - comment out for production)
    if (DEBUG_SENSOR_DATA && (currentTime % 1000 == 0)) {
      printSensorData();
    }
  }

  delay(MAIN_LOOP_DELAY_MS);
}

void readSensors() {
  currentSensorData.timestamp = millis();
  currentSensorData.valid = true;

  // Read IMU (MPU6050)
  if (imuSensor.isInitialized()) {
    float temp;
    imuSensor.readData(currentSensorData.accel_x,
                       currentSensorData.accel_y,
                       currentSensorData.accel_z,
                       currentSensorData.gyro_x,
                       currentSensorData.gyro_y,
                       currentSensorData.gyro_z,
                       temp);
  } else {
    currentSensorData.accel_x = 0;
    currentSensorData.accel_y = 0;
    currentSensorData.accel_z = 1.0;  // 1g gravity
    currentSensorData.gyro_x = 0;
    currentSensorData.gyro_y = 0;
    currentSensorData.gyro_z = 0;
  }

  // Read pressure sensor (BMP280)
  if (pressureSensor.isInitialized()) {
    float temp, altitude;
    pressureSensor.readData(temp, currentSensorData.pressure, altitude);
  } else {
    currentSensorData.pressure = 1013.25;  // Sea level pressure
  }

  // Read heart rate (MAX30102)
  if (heartRateSensor.isInitialized()) {
    float bpm;
    bool fingerDetected;
    if (heartRateSensor.readHeartRate(bpm, fingerDetected)) {
      currentSensorData.heart_rate = bpm;
    } else {
      currentSensorData.heart_rate = 0;
    }
  } else {
    currentSensorData.heart_rate = 0;
  }

  // Read force sensor (FSR)
  if (forceSensor.isInitialized()) {
    currentSensorData.fsr_value = forceSensor.readRaw();
  } else {
    currentSensorData.fsr_value = 0;
  }
}

void handleFallDetected() {
  Serial.println("\n!!! FALL DETECTED !!!");

  // Get confidence score from fall detector
  uint8_t confidence = confidenceScorer.getTotalScore();

  Serial.print("Confidence Score: ");
  Serial.print(confidence);
  Serial.println("/105");

  // Activate alerts based on confidence
  if (confidence >= HIGH_CONFIDENCE_THRESHOLD) {
    Serial.println("HIGH CONFIDENCE FALL - Immediate Alert");
    activateAlerts(true);
  } else if (confidence >= CONFIRMED_THRESHOLD) {
    Serial.println("CONFIRMED FALL - Delayed Alert");
    activateAlerts(false);
  }

  // Print detailed fall information
  fallDetector.printStageDetails();

  // Wait for user response (simplified for Arduino)
  delay(5000);

  // Reset detection
  fallDetector.resetDetection();
  deactivateAlerts();
}

void handleSOSButton() {
  Serial.println("\n!!! SOS BUTTON PRESSED !!!");
  activateAlerts(true);

  // Wait for button release
  while (digitalRead(SOS_BUTTON_PIN) == LOW) {
    delay(100);
  }

  delay(5000);  // Keep alerts active
  deactivateAlerts();
}

void activateAlerts(bool immediate) {
  digitalWrite(VISUAL_ALERT_PIN, HIGH);
  digitalWrite(HAPTIC_PIN, HIGH);

  // Beep pattern
  for (int i = 0; i < 3; i++) {
    digitalWrite(SPEAKER_PIN, HIGH);
    delay(ALERT_BEEP_DURATION_MS);
    digitalWrite(SPEAKER_PIN, LOW);
    delay(200);
  }
}

void deactivateAlerts() {
  digitalWrite(SPEAKER_PIN, LOW);
  digitalWrite(HAPTIC_PIN, LOW);
  digitalWrite(VISUAL_ALERT_PIN, LOW);
}

void printSensorData() {
  Serial.println("--- Sensor Data ---");
  Serial.print("Accel (g): ");
  Serial.print(currentSensorData.accel_x, 2);
  Serial.print(", ");
  Serial.print(currentSensorData.accel_y, 2);
  Serial.print(", ");
  Serial.println(currentSensorData.accel_z, 2);

  Serial.print("Gyro (°/s): ");
  Serial.print(currentSensorData.gyro_x, 2);
  Serial.print(", ");
  Serial.print(currentSensorData.gyro_y, 2);
  Serial.print(", ");
  Serial.println(currentSensorData.gyro_z, 2);

  Serial.print("Pressure: ");
  Serial.print(currentSensorData.pressure, 2);
  Serial.println(" hPa");

  Serial.print("Heart Rate: ");
  Serial.print(currentSensorData.heart_rate, 1);
  Serial.println(" BPM");

  Serial.print("FSR: ");
  Serial.println(currentSensorData.fsr_value);

  Serial.println();
}
