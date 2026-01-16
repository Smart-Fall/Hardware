#include "MAX30102_Sensor.h"

MAX30102Sensor::MAX30102Sensor(uint8_t sda, uint8_t scl)
    : heartRateSensor(&Wire, 0x57), initialized(false), sda_pin(sda), scl_pin(scl),
      buffer_index(0), last_read_time(0), baseline_heart_rate(60), baseline_spo2(95), baseline_set(false)
{
    // Initialize buffers
    memset(heart_rate_buffer, 0, sizeof(heart_rate_buffer));
    memset(spo2_buffer, 0, sizeof(spo2_buffer));
}

bool MAX30102Sensor::begin(uint8_t address)
{
    // Initialize I2C if not already done
    if (!Wire.begin(sda_pin, scl_pin))
    {
        Serial.println("Failed to initialize I2C for MAX30102");
        return false;
    }

    // Initialize sensor with retries
    uint8_t retries = 3;
    while (retries-- > 0)
    {
        if (heartRateSensor.begin())
        {
            initialized = true;
            Serial.println("MAX30102 sensor initialized successfully");

            // Start data collection
            startCollection();
            delay(100);
            return true;
        }
        delay(500);
    }

    Serial.println("ERROR: MAX30102 initialization failed after retries");
    initialized = false;
    return false;
}

void MAX30102Sensor::configure()
{
    if (!initialized)
        return;

    // Sensor starts data collection automatically via startCollection()
    Serial.println("MAX30102 configured");
}

bool MAX30102Sensor::readData(uint16_t &heart_rate, uint8_t &spo2, float &temperature)
{
    if (!initialized)
        return false;

    uint32_t currentTime = millis();

    // Sensor updates every 4 seconds per documentation
    if (currentTime - last_read_time < 4000)
    {
        return false; // No new data yet
    }

    last_read_time = currentTime;

    // Read sensor data
    heartRateSensor.getHeartbeatSPO2();

    heart_rate = heartRateSensor._sHeartbeatSPO2.Heartbeat;
    spo2 = heartRateSensor._sHeartbeatSPO2.SPO2;
    temperature = heartRateSensor.getTemperature_C();

    // Validate ranges (typical healthy values)
    if (heart_rate < 30 || heart_rate > 200)
    {
        Serial.print("WARNING: Heart rate out of range: ");
        Serial.println(heart_rate);
        return false;
    }

    if (spo2 < 80 || spo2 > 100)
    {
        Serial.print("WARNING: SpO2 out of range: ");
        Serial.println(spo2);
        return false;
    }

    // Buffer values for stability
    heart_rate_buffer[buffer_index] = heart_rate;
    spo2_buffer[buffer_index] = spo2;
    buffer_index = (buffer_index + 1) % 5;

    return true;
}

bool MAX30102Sensor::getRawData(uint16_t &heart_rate, uint8_t &spo2)
{
    if (!initialized)
        return false;

    heartRateSensor.getHeartbeatSPO2();

    heart_rate = heartRateSensor._sHeartbeatSPO2.Heartbeat;
    spo2 = heartRateSensor._sHeartbeatSPO2.SPO2;

    return true;
}

float MAX30102Sensor::getTemperature()
{
    if (!initialized)
        return -1.0f;

    return heartRateSensor.getTemperature_C();
}

void MAX30102Sensor::setBaselineHeartRate(uint16_t baseline)
{
    baseline_heart_rate = baseline;
    baseline_set = true;
}

void MAX30102Sensor::setBaselineSPO2(uint8_t baseline)
{
    baseline_spo2 = baseline;
}

uint16_t MAX30102Sensor::getBaselineHeartRate()
{
    return baseline_heart_rate;
}

uint8_t MAX30102Sensor::getBaselineSPO2()
{
    return baseline_spo2;
}

void MAX30102Sensor::resetBaseline()
{
    baseline_heart_rate = 60;
    baseline_spo2 = 95;
    baseline_set = false;
    Serial.println("MAX30102 baseline reset");
}

void MAX30102Sensor::startCollection()
{
    if (!initialized)
        return;

    heartRateSensor.sensorStartCollect();
    Serial.println("MAX30102 data collection started");
}

void MAX30102Sensor::stopCollection()
{
    if (!initialized)
        return;

    heartRateSensor.sensorEndCollect();
    Serial.println("MAX30102 data collection stopped");
}

bool MAX30102Sensor::isInitialized()
{
    return initialized;
}

void MAX30102Sensor::printInfo()
{
    if (!initialized)
    {
        Serial.println("MAX30102 sensor not initialized");
        return;
    }

    Serial.println("\n--- MAX30102 Sensor Info ---");
    Serial.print("Status: ");
    Serial.println(initialized ? "INITIALIZED" : "NOT INITIALIZED");
    Serial.print("Baseline Heart Rate: ");
    Serial.print(baseline_heart_rate);
    Serial.println(" BPM");
    Serial.print("Baseline SpO2: ");
    Serial.print(baseline_spo2);
    Serial.println(" %");
    Serial.print("Temperature: ");
    Serial.print(getTemperature());
    Serial.println(" °C");
    Serial.println("----------------------------\n");
}
