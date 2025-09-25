#include "fsr_sensor.h"

FSRSensor::FSRSensor() : analog_pin(FSR_ANALOG_PIN), initialized(false),
                         baseline_value(0), impact_threshold(500), strap_threshold(100),
                         buffer_index(0), last_read_time(0), last_impact_time(0) {
    // Initialize reading buffer
    for(int i = 0; i < 5; i++) {
        reading_buffer[i] = 0;
    }
}

FSRSensor::~FSRSensor() {
    // Cleanup if needed
}

bool FSRSensor::init(uint8_t pin) {
    analog_pin = pin;
    pinMode(analog_pin, INPUT);

    initialized = true;
    Serial.print("FSR sensor (Potentiometer simulation) initialized on pin ");
    Serial.println(analog_pin);

    // Calibrate baseline
    calibrateBaseline();

    return true;
}

bool FSRSensor::isInitialized() const {
    return initialized;
}

uint16_t FSRSensor::readRawValue() {
    if (!initialized) return 0;

    uint16_t reading = analogRead(analog_pin);

    // Update rolling buffer for impact detection
    reading_buffer[buffer_index] = reading;
    buffer_index = (buffer_index + 1) % 5;

    last_read_time = millis();
    return reading;
}

float FSRSensor::readPressure() {
    if (!initialized) return 0.0f;

    uint16_t raw_value = readRawValue();

    // Convert raw ADC reading to pressure approximation
    // FSR resistance changes non-linearly with force
    // This is a simplified conversion for simulation
    if (raw_value < baseline_value + 50) {
        return 0.0f;  // No significant pressure
    }

    // Map to pressure range (0-100 N approximation)
    float pressure = map(raw_value - baseline_value, 0, 4095 - baseline_value, 0, 100);
    return constrain(pressure, 0.0f, 100.0f);
}

bool FSRSensor::detectImpact() {
    if (!initialized) return false;

    uint16_t current_reading = readRawValue();

    // Impact detection: sudden spike above threshold
    if (current_reading > (baseline_value + impact_threshold)) {
        // Check if this is a new impact (not just sustained pressure)
        if ((millis() - last_impact_time) > 500) {  // 500ms minimum between impacts
            last_impact_time = millis();
            return true;
        }
    }

    return false;
}

bool FSRSensor::isStrapSecure() {
    if (!initialized) return false;

    uint16_t current_reading = readRawValue();

    // Strap is considered secure if there's consistent baseline pressure
    // indicating the device is properly attached
    return (current_reading >= (baseline_value + strap_threshold));
}

void FSRSensor::calibrateBaseline() {
    if (!initialized) return;

    Serial.println("Calibrating FSR baseline... Remove all pressure!");

    delay(2000);  // Wait for user to remove pressure

    uint32_t sum = 0;
    int samples = 50;

    for (int i = 0; i < samples; i++) {
        sum += analogRead(analog_pin);
        delay(50);
    }

    baseline_value = sum / samples;

    Serial.print("FSR baseline calibrated to: ");
    Serial.print(baseline_value);
    Serial.println(" ADC counts");

    // Set reasonable thresholds based on baseline
    impact_threshold = max(500, (int)(baseline_value * 0.5f));  // 50% above baseline minimum
    strap_threshold = max(50, (int)(baseline_value * 0.1f));    // 10% above baseline

    Serial.print("Impact threshold: ");
    Serial.println(baseline_value + impact_threshold);
    Serial.print("Strap threshold: ");
    Serial.println(baseline_value + strap_threshold);
}

void FSRSensor::setImpactThreshold(uint16_t threshold) {
    impact_threshold = threshold;
    Serial.print("Impact threshold set to: ");
    Serial.println(baseline_value + threshold);
}

void FSRSensor::setStrapThreshold(uint16_t threshold) {
    strap_threshold = threshold;
    Serial.print("Strap threshold set to: ");
    Serial.println(baseline_value + threshold);
}

bool FSRSensor::isDataReady() {
    if (!initialized) return false;

    // FSR can be read at high frequency
    return (millis() - last_read_time) >= 10;  // 100Hz capability
}

uint16_t FSRSensor::getBaseline() const {
    return baseline_value;
}

uint32_t FSRSensor::getTimeSinceLastImpact() {
    return millis() - last_impact_time;
}

void FSRSensor::printSensorInfo() {
    if (!initialized) {
        Serial.println("FSR sensor not initialized");
        return;
    }

    Serial.println("=== FSR Sensor Info ===");
    Serial.println("Sensor: Potentiometer (Wokwi simulation)");
    Serial.print("Analog Pin: ");
    Serial.println(analog_pin);
    Serial.print("Baseline: ");
    Serial.print(baseline_value);
    Serial.println(" ADC counts");
    Serial.print("Impact Threshold: ");
    Serial.print(baseline_value + impact_threshold);
    Serial.println(" ADC counts");
    Serial.print("Strap Threshold: ");
    Serial.print(baseline_value + strap_threshold);
    Serial.println(" ADC counts");
    Serial.println("=======================");
}

void FSRSensor::printRawData() {
    if (!initialized) return;

    uint16_t raw = readRawValue();
    float pressure = readPressure();
    bool impact = detectImpact();
    bool strap_ok = isStrapSecure();

    Serial.print("FSR Raw: ");
    Serial.print(raw);
    Serial.print(" | Pressure: ");
    Serial.print(pressure, 1);
    Serial.print(" N | Impact: ");
    Serial.print(impact ? "YES" : "NO");
    Serial.print(" | Strap: ");
    Serial.println(strap_ok ? "SECURE" : "LOOSE");
}