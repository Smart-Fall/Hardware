#include "max30102_sensor.h"

MAX30102Sensor::MAX30102Sensor() : initialized(false), analog_pin(MAX30102_SIM_PIN),
                                   baseline_heart_rate(70.0f), buffer_index(0),
                                   last_read_time(0), last_beat_time(0),
                                   simulated_bpm(70.0f), bpm_variation(5.0f),
                                   simulation_start_time(0) {
    // Initialize heart rate buffer
    for(int i = 0; i < 10; i++) {
        heart_rate_buffer[i] = baseline_heart_rate;
    }
}

MAX30102Sensor::~MAX30102Sensor() {
    // Cleanup if needed
}

bool MAX30102Sensor::init() {
    pinMode(analog_pin, INPUT);

    // Initialize simulation
    simulation_start_time = millis();
    initialized = true;

    Serial.println("MAX30102 sensor (Potentiometer simulation) initialized successfully");

    // Calibrate baseline
    calibrateBaseline();

    return true;
}

bool MAX30102Sensor::isInitialized() const {
    return initialized;
}

bool MAX30102Sensor::readHeartRate(float& bpm) {
    if (!initialized) return false;

    // Read potentiometer value (0-4095 for ESP32)
    int analog_value = analogRead(analog_pin);

    // Convert analog reading to heart rate (40-180 BPM range)
    // This simulates heart rate sensor readings
    bpm = map(analog_value, 0, 4095, 40, 180);

    // Add some realistic variation and smoothing
    uint32_t current_time = millis();
    float time_factor = sin((current_time - simulation_start_time) / 1000.0f) * bpm_variation;
    bpm = simulated_bpm + time_factor + random(-2, 3);

    // Ensure realistic heart rate bounds
    bpm = constrain(bpm, 40.0f, 180.0f);

    // Update rolling average buffer
    heart_rate_buffer[buffer_index] = bpm;
    buffer_index = (buffer_index + 1) % 10;

    last_read_time = current_time;

    // Simulate beat detection timing
    if (bpm > 0) {
        uint32_t beat_interval = (60.0f / bpm) * 1000.0f;  // Convert BPM to ms
        if (current_time - last_beat_time >= beat_interval) {
            last_beat_time = current_time;
        }
    }

    return true;
}

float MAX30102Sensor::getAverageHeartRate() {
    if (!initialized) return 0.0f;

    float sum = 0.0f;
    for(int i = 0; i < 10; i++) {
        sum += heart_rate_buffer[i];
    }
    return sum / 10.0f;
}

bool MAX30102Sensor::isHeartRateValid() {
    if (!initialized) return false;

    float avg_hr = getAverageHeartRate();

    // Valid heart rate range: 40-180 BPM
    return (avg_hr >= 40.0f && avg_hr <= 180.0f);
}

float MAX30102Sensor::getHeartRateBaseline() {
    return baseline_heart_rate;
}

float MAX30102Sensor::getHeartRateChange() {
    if (!initialized) return 0.0f;

    float current_avg = getAverageHeartRate();
    return current_avg - baseline_heart_rate;
}

void MAX30102Sensor::calibrateBaseline() {
    if (!initialized) return;

    Serial.println("Calibrating heart rate baseline... Keep still for 10 seconds!");

    float sum = 0.0f;
    int valid_readings = 0;

    for (int i = 0; i < 50; i++) {  // 10 seconds at 5Hz
        float bpm;
        if (readHeartRate(bpm)) {
            sum += bpm;
            valid_readings++;
        }
        delay(200);  // 5Hz reading rate for calibration
    }

    if (valid_readings > 0) {
        baseline_heart_rate = sum / valid_readings;
        Serial.print("Heart rate baseline set to: ");
        Serial.print(baseline_heart_rate, 1);
        Serial.println(" BPM");
    }
}

void MAX30102Sensor::setSimulatedHeartRate(float bpm, float variation) {
    simulated_bpm = constrain(bpm, 40.0f, 180.0f);
    bpm_variation = constrain(variation, 0.0f, 20.0f);

    Serial.print("Simulated heart rate set to: ");
    Serial.print(simulated_bpm, 1);
    Serial.print(" ± ");
    Serial.print(bpm_variation, 1);
    Serial.println(" BPM");
}

void MAX30102Sensor::simulateStressResponse(float increase_bpm) {
    float new_bpm = simulated_bpm + increase_bpm;
    setSimulatedHeartRate(new_bpm, bpm_variation * 1.5f);  // Increase variation during stress

    Serial.print("Stress response simulated - HR increased by ");
    Serial.print(increase_bpm, 1);
    Serial.println(" BPM");
}

bool MAX30102Sensor::isDataReady() {
    if (!initialized) return false;

    // Heart rate sensor typically updates at 1-5Hz
    return (millis() - last_read_time) >= 200;  // 5Hz update rate
}

uint32_t MAX30102Sensor::getTimeSinceLastBeat() {
    return millis() - last_beat_time;
}

void MAX30102Sensor::printSensorInfo() {
    if (!initialized) {
        Serial.println("MAX30102 sensor not initialized");
        return;
    }

    Serial.println("=== MAX30102 Sensor Info ===");
    Serial.println("Sensor: Potentiometer (Wokwi simulation)");
    Serial.print("Analog Pin: ");
    Serial.println(analog_pin);
    Serial.print("Baseline HR: ");
    Serial.print(baseline_heart_rate, 1);
    Serial.println(" BPM");
    Serial.print("Simulated HR: ");
    Serial.print(simulated_bpm, 1);
    Serial.print(" ± ");
    Serial.print(bpm_variation, 1);
    Serial.println(" BPM");
    Serial.println("============================");
}

void MAX30102Sensor::printRawData() {
    if (!initialized) return;

    float bpm;
    if (readHeartRate(bpm)) {
        float avg_hr = getAverageHeartRate();
        float hr_change = getHeartRateChange();

        Serial.print("HR: ");
        Serial.print(bpm, 1); Serial.print(" BPM");

        Serial.print(" | Avg: ");
        Serial.print(avg_hr, 1); Serial.print(" BPM");

        Serial.print(" | Change: ");
        Serial.print(hr_change > 0 ? "+" : "");
        Serial.print(hr_change, 1); Serial.print(" BPM");

        Serial.print(" | Valid: ");
        Serial.println(isHeartRateValid() ? "YES" : "NO");
    }
}