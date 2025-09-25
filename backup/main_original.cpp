#include <Arduino.h>

// Utility includes
#include "utils/config.h"
#include "utils/data_types.h"

// Sensor module includes
#include "sensors/bmi323_sensor.h"
#include "sensors/bmp280_sensor.h"
#include "sensors/max30102_sensor.h"
#include "sensors/fsr_sensor.h"

// Detection algorithm includes
#include "detection/fall_detector.h"
#include "detection/confidence_scorer.h"

// System components
BMI323Sensor imuSensor;
BMP280Sensor pressureSensor;
MAX30102Sensor heartRateSensor;
FSRSensor fsrSensor;

FallDetector fallDetector;
ConfidenceScorer confidenceScorer;

// System state variables
bool system_initialized = false;
uint32_t last_sensor_read = 0;
uint32_t last_heartbeat = 0;
uint32_t system_start_time = 0;

// Alert system pins (LEDs for simulation)
bool audio_alert_active = false;
bool haptic_alert_active = false;
bool visual_alert_active = false;

// SOS button handling
volatile bool sos_button_pressed = false;
uint32_t last_button_press = 0;

// Forward declarations
void IRAM_ATTR sosButtonISR();
bool initializeAllSensors();
void readAllSensors(SensorData_t& data);
void processDetectionLogic(SensorData_t& data);
void handleSOSButton();
void updateAlertSystem();
void systemHeartbeat();
void printSystemStatus();

void setup() {
    Serial.begin(SERIAL_BAUD_RATE);
    delay(2000);  // Wait for serial connection

    Serial.println("=================================");
    Serial.println("    SmartFall Detection System");
    Serial.println("      Wokwi Simulation Mode");
    Serial.println("=================================");
    Serial.print("Version: ");
    Serial.println("1.0.0");
    Serial.print("Sample Rate: ");
    Serial.print(SENSOR_SAMPLE_RATE_HZ);
    Serial.println(" Hz");
    Serial.println();

    // Initialize alert system pins (LEDs for simulation)
    pinMode(SPEAKER_PIN, OUTPUT);        // Yellow LED (audio alert)
    pinMode(HAPTIC_PIN, OUTPUT);         // Blue LED (haptic alert)
    pinMode(VISUAL_ALERT_PIN, OUTPUT);   // Red LED (visual alert)

    digitalWrite(SPEAKER_PIN, LOW);
    digitalWrite(HAPTIC_PIN, LOW);
    digitalWrite(VISUAL_ALERT_PIN, LOW);

    // Initialize SOS button with interrupt
    pinMode(SOS_BUTTON_PIN, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(SOS_BUTTON_PIN), sosButtonISR, FALLING);

    // Initialize all sensors
    Serial.println("Initializing sensors...");
    system_initialized = initializeAllSensors();

    if (system_initialized) {
        Serial.println("✓ All sensors initialized successfully");

        // Initialize fall detection system
        if (fallDetector.init()) {
            Serial.println("✓ Fall detection algorithm initialized");
        } else {
            Serial.println("✗ Fall detection initialization failed");
            system_initialized = false;
        }

        confidenceScorer.resetScore();
        Serial.println("✓ Confidence scoring system ready");

    } else {
        Serial.println("✗ Sensor initialization failed");
        Serial.println("System will continue with limited functionality");
    }

    system_start_time = millis();
    last_sensor_read = system_start_time;
    last_heartbeat = system_start_time;

    Serial.println();
    Serial.println("=================================");
    Serial.println("    System Ready - Monitoring");
    Serial.println("=================================");
    Serial.println("Legend:");
    Serial.println("  Yellow LED: Audio Alert");
    Serial.println("  Blue LED:   Haptic Alert");
    Serial.println("  Red LED:    Visual Alert");
    Serial.println("  Red Button: SOS Emergency");
    Serial.println("=================================");
    Serial.println();
}

void loop() {
    uint32_t current_time = millis();

    // Handle SOS button press (highest priority)
    if (sos_button_pressed) {
        handleSOSButton();
    }

    // Read sensors at specified sample rate
    if (system_initialized && (current_time - last_sensor_read >= (1000 / SENSOR_SAMPLE_RATE_HZ))) {
        SensorData_t sensor_data = {0};

        // Read all sensor data
        readAllSensors(sensor_data);

        if (sensor_data.valid) {
            // Process fall detection logic
            processDetectionLogic(sensor_data);

            // Update alert system based on detection status
            updateAlertSystem();
        }

        last_sensor_read = current_time;
    }

    // System heartbeat (status updates)
    if (current_time - last_heartbeat >= HEARTBEAT_INTERVAL_MS) {
        systemHeartbeat();
        last_heartbeat = current_time;
    }

    // Small delay to prevent excessive CPU usage
    delay(MAIN_LOOP_DELAY_MS);
}

void IRAM_ATTR sosButtonISR() {
    uint32_t current_time = millis();

    // Debounce button press (50ms minimum)
    if (current_time - last_button_press > 50) {
        sos_button_pressed = true;
        last_button_press = current_time;
    }
}

bool initializeAllSensors() {
    bool all_sensors_ok = true;

    Serial.println("Initializing BMI-323 (MPU6050 simulation)...");
    if (!imuSensor.init()) {
        Serial.println("✗ IMU sensor initialization failed");
        all_sensors_ok = false;
    }

    Serial.println("Initializing BMP-280 (DHT22 simulation)...");
    if (!pressureSensor.init()) {
        Serial.println("✗ Pressure sensor initialization failed");
        all_sensors_ok = false;
    }

    Serial.println("Initializing MAX30102 (Potentiometer simulation)...");
    if (!heartRateSensor.init()) {
        Serial.println("✗ Heart rate sensor initialization failed");
        all_sensors_ok = false;
    }

    Serial.println("Initializing FSR (Potentiometer simulation)...");
    if (!fsrSensor.init()) {
        Serial.println("✗ FSR sensor initialization failed");
        all_sensors_ok = false;
    }

    return all_sensors_ok;
}

void readAllSensors(SensorData_t& data) {
    data.timestamp = millis();
    data.valid = true;

    // Read IMU data (acceleration and gyroscope)
    if (imuSensor.isInitialized()) {
        bool accel_ok = imuSensor.readAcceleration(data.accel_x, data.accel_y, data.accel_z);
        bool gyro_ok = imuSensor.readAngularVelocity(data.gyro_x, data.gyro_y, data.gyro_z);
        data.valid &= (accel_ok && gyro_ok);
    }

    // Read environmental data (pressure simulation)
    if (pressureSensor.isInitialized()) {
        data.pressure = pressureSensor.readPressure();
    }

    // Read heart rate data
    if (heartRateSensor.isInitialized()) {
        float hr;
        if (heartRateSensor.readHeartRate(hr)) {
            data.heart_rate = hr;
        }
    }

    // Read FSR data
    if (fsrSensor.isInitialized()) {
        data.fsr_value = fsrSensor.readRawValue();
    }

    // Debug sensor data (if enabled)
    if (DEBUG_SENSOR_DATA && data.valid) {
        Serial.print("Sensors - Accel: ");
        Serial.print(data.accel_x, 2); Serial.print(",");
        Serial.print(data.accel_y, 2); Serial.print(",");
        Serial.print(data.accel_z, 2);
        Serial.print(" | Gyro: ");
        Serial.print(data.gyro_x, 1); Serial.print(",");
        Serial.print(data.gyro_y, 1); Serial.print(",");
        Serial.print(data.gyro_z, 1);
        Serial.print(" | HR: ");
        Serial.print(data.heart_rate, 0);
        Serial.print(" | FSR: ");
        Serial.println(data.fsr_value);
    }
}

void processDetectionLogic(SensorData_t& data) {
    // Process sensor data through fall detection algorithm
    fallDetector.processSensorData(data);

    FallStatus_t current_status = fallDetector.getCurrentStatus();

    // Handle different detection states
    switch (current_status) {
        case FALL_STATUS_POTENTIAL_FALL:
            {
                Serial.println(">>> POTENTIAL FALL DETECTED - Analyzing...");

                // Calculate confidence score based on detection stages
                confidenceScorer.addStage1Score(fallDetector.getFreefalDuration(),
                                               sqrt(data.accel_x*data.accel_x + data.accel_y*data.accel_y + data.accel_z*data.accel_z));
                confidenceScorer.addStage2Score(fallDetector.getMaxImpact(),
                                               data.timestamp, fsrSensor.detectImpact());
                confidenceScorer.addStage3Score(fallDetector.getMaxRotation(), 0);
                confidenceScorer.addStage4Score(2000, true);  // Simulated inactivity

                // Add filter scores
                confidenceScorer.addPressureFilterScore(abs(pressureSensor.getAltitudeChange()));
                confidenceScorer.addHeartRateFilterScore(heartRateSensor.getHeartRateChange());
                confidenceScorer.addFSRFilterScore(fsrSensor.detectImpact(), fsrSensor.isStrapSecure());

                // Evaluate confidence
                uint8_t total_score = confidenceScorer.getTotalScore();
                FallConfidence_t confidence = confidenceScorer.getConfidenceLevel();

                Serial.print("Confidence Score: ");
                Serial.print(total_score);
                Serial.print("/105 - ");
                Serial.println(confidenceScorer.getConfidenceString(confidence));

                if (confidence >= CONFIDENCE_CONFIRMED) {
                    Serial.println(">>> FALL CONFIRMED - EMERGENCY ALERT ACTIVATED!");
                    // This would trigger the emergency alert sequence
                    visual_alert_active = true;
                    audio_alert_active = true;
                    haptic_alert_active = true;
                }

                confidenceScorer.printScoreBreakdown();
                break;
            }

        case FALL_STATUS_STAGE1_FREEFALL:
        case FALL_STATUS_STAGE2_IMPACT:
        case FALL_STATUS_STAGE3_ROTATION:
        case FALL_STATUS_STAGE4_INACTIVITY:
            // Detection in progress - no action needed, just continue monitoring
            break;

        case FALL_STATUS_MONITORING:
        default:
            // Normal monitoring - reset any alert states
            audio_alert_active = false;
            haptic_alert_active = false;
            visual_alert_active = false;
            break;
    }
}

void handleSOSButton() {
    sos_button_pressed = false;  // Clear the flag

    Serial.println();
    Serial.println(">>> SOS BUTTON PRESSED - MANUAL EMERGENCY ACTIVATED!");
    Serial.println(">>> Bypassing all detection stages");
    Serial.println(">>> EMERGENCY ALERT ACTIVATED!");

    // Activate all alert systems immediately
    audio_alert_active = true;
    haptic_alert_active = true;
    visual_alert_active = true;

    // Reset fall detector and start fresh monitoring
    fallDetector.resetDetection();
    confidenceScorer.resetScore();

    Serial.println(">>> Emergency services would be contacted now");
    Serial.println();
}

void updateAlertSystem() {
    // Update LED states based on alert flags
    digitalWrite(SPEAKER_PIN, audio_alert_active ? HIGH : LOW);
    digitalWrite(HAPTIC_PIN, haptic_alert_active ? HIGH : LOW);
    digitalWrite(VISUAL_ALERT_PIN, visual_alert_active ? HIGH : LOW);

    // Auto-reset alerts after timeout (for simulation)
    static uint32_t alert_start_time = 0;
    if (audio_alert_active || haptic_alert_active || visual_alert_active) {
        if (alert_start_time == 0) {
            alert_start_time = millis();
        } else if ((millis() - alert_start_time) > 10000) {  // 10 second demo timeout
            audio_alert_active = false;
            haptic_alert_active = false;
            visual_alert_active = false;
            alert_start_time = 0;
            Serial.println("Alert timeout - returning to monitoring mode");
        }
    } else {
        alert_start_time = 0;
    }
}

void systemHeartbeat() {
    // Print system status every few seconds
    static uint8_t heartbeat_counter = 0;
    heartbeat_counter++;

    if (heartbeat_counter >= 5) {  // Every 5 seconds
        printSystemStatus();
        heartbeat_counter = 0;
    }
}

void printSystemStatus() {
    uint32_t uptime = millis() - system_start_time;
    FallStatus_t status = fallDetector.getCurrentStatus();

    Serial.println("--- System Status ---");
    Serial.print("Uptime: ");
    Serial.print(uptime / 1000);
    Serial.print("s | Status: ");
    Serial.print(fallDetector.getStatusString(status));

    if (imuSensor.isInitialized()) {
        float total_accel = imuSensor.getTotalAcceleration();
        float angular_mag = imuSensor.getAngularMagnitude();
        Serial.print(" | Accel: ");
        Serial.print(total_accel, 2);
        Serial.print("g | Gyro: ");
        Serial.print(angular_mag, 1);
        Serial.print("°/s");
    }

    Serial.println();
}