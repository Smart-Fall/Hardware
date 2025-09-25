#include "bmp280_sensor.h"

BMP280Sensor::BMP280Sensor() : dht(DHT22_PIN, DHT22), initialized(false),
                               baseline_pressure(1013.25f), baseline_altitude(0.0f),
                               last_read_time(0), history_index(0) {
    // Initialize altitude history
    for(int i = 0; i < 10; i++) {
        altitude_history[i] = 0.0f;
    }
}

BMP280Sensor::~BMP280Sensor() {
    // Cleanup if needed
}

bool BMP280Sensor::init() {
    dht.begin();

    // Wait for sensor to stabilize
    delay(2000);

    // Test reading
    float temp = dht.readTemperature();
    if (isnan(temp)) {
        Serial.println("Failed to initialize DHT22 (BMP-280 simulation)");
        return false;
    }

    initialized = true;
    Serial.println("BMP-280 sensor (DHT22) initialized successfully");

    // Set initial baseline
    resetAltitudeBaseline();

    return true;
}

bool BMP280Sensor::isInitialized() const {
    return initialized;
}

float BMP280Sensor::readPressure() {
    if (!initialized) return 0.0f;

    // Simulate pressure readings based on temperature variations
    // DHT22 doesn't measure pressure, so we simulate it
    float temp = dht.readTemperature();
    if (isnan(temp)) return baseline_pressure;

    // Create simulated pressure variations
    // In real implementation, this would be actual BMP-280 pressure reading
    float simulated_pressure = baseline_pressure + (temp - 25.0f) * 0.5f;

    last_read_time = millis();
    return simulated_pressure;
}

float BMP280Sensor::readTemperature() {
    if (!initialized) return 0.0f;

    float temp = dht.readTemperature();
    return isnan(temp) ? 25.0f : temp;  // Default to 25°C if reading fails
}

float BMP280Sensor::readAltitude() {
    if (!initialized) return 0.0f;

    float pressure = readPressure();
    float altitude = pressureToAltitude(pressure, baseline_pressure);

    // Update rolling average for stability
    altitude_history[history_index] = altitude;
    history_index = (history_index + 1) % 10;

    return altitude;
}

float BMP280Sensor::getAltitudeChange() {
    if (!initialized) return 0.0f;

    float current_altitude = readAltitude();
    return current_altitude - baseline_altitude;
}

void BMP280Sensor::resetAltitudeBaseline() {
    if (!initialized) return;

    // Take several readings for stable baseline
    float altitude_sum = 0.0f;
    int valid_readings = 0;

    for (int i = 0; i < 10; i++) {
        float altitude = readAltitude();
        if (!isnan(altitude)) {
            altitude_sum += altitude;
            valid_readings++;
        }
        delay(100);
    }

    if (valid_readings > 0) {
        baseline_altitude = altitude_sum / valid_readings;
        Serial.print("Altitude baseline set to: ");
        Serial.print(baseline_altitude, 2);
        Serial.println(" m");
    }
}

void BMP280Sensor::setSeaLevelPressure(float pressure_hpa) {
    baseline_pressure = pressure_hpa;
    Serial.print("Sea level pressure set to: ");
    Serial.print(baseline_pressure, 2);
    Serial.println(" hPa");
}

bool BMP280Sensor::isDataReady() {
    if (!initialized) return false;

    // DHT22 has ~2 second update rate
    return (millis() - last_read_time) >= 2000;
}

float BMP280Sensor::pressureToAltitude(float pressure_hpa, float sea_level_hpa) {
    // Standard atmospheric pressure formula
    // altitude = 44330 * (1 - (pressure/sea_level)^(1/5.255))

    if (pressure_hpa <= 0 || sea_level_hpa <= 0) return 0.0f;

    return 44330.0f * (1.0f - pow(pressure_hpa / sea_level_hpa, 1.0f / 5.255f));
}

void BMP280Sensor::printSensorInfo() {
    if (!initialized) {
        Serial.println("BMP-280 sensor not initialized");
        return;
    }

    Serial.println("=== BMP-280 Sensor Info ===");
    Serial.println("Sensor: DHT22 (Wokwi simulation)");
    Serial.print("Baseline Pressure: ");
    Serial.print(baseline_pressure, 2);
    Serial.println(" hPa");
    Serial.print("Baseline Altitude: ");
    Serial.print(baseline_altitude, 2);
    Serial.println(" m");
    Serial.println("===========================");
}

void BMP280Sensor::printRawData() {
    if (!initialized) return;

    float pressure = readPressure();
    float temperature = readTemperature();
    float altitude = readAltitude();
    float altitude_change = getAltitudeChange();

    Serial.print("Pressure: ");
    Serial.print(pressure, 2); Serial.print(" hPa");

    Serial.print(" | Temp: ");
    Serial.print(temperature, 1); Serial.print(" °C");

    Serial.print(" | Alt: ");
    Serial.print(altitude, 2); Serial.print(" m");

    Serial.print(" | Change: ");
    Serial.print(altitude_change, 2); Serial.println(" m");
}