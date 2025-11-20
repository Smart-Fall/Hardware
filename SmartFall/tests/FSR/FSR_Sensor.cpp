#include "FSR_Sensor.h"

FSR_Sensor::FSR_Sensor(uint8_t pin)
    : analog_pin(pin), initialized(false), baseline_value(0)
{
}

bool FSR_Sensor::begin()
{
    pinMode(analog_pin, INPUT);
    initialized = true;
    return true;
}

uint16_t FSR_Sensor::readRaw()
{
    if (!initialized)
        return 0;
    return analogRead(analog_pin);
}

float FSR_Sensor::readForce()
{
    if (!initialized)
        return 0.0;

    uint16_t raw = analogRead(analog_pin);

    // Approximate conversion to force (Newtons)
    // This is a simplified model - adjust based on your FSR datasheet
    if (raw < 10)
        return 0.0;

    float voltage = (raw / 4095.0) * 3.3;
    float resistance = (3.3 - voltage) / voltage * 10000; // Assuming 10K pull-down
    float force = 0.0;

    // Approximate force calculation (FSR402 typical)
    if (resistance < 1000)
    {
        force = 10.0; // >10N
    }
    else
    {
        force = 10000.0 / resistance; // Rough approximation
    }

    return force;
}

bool FSR_Sensor::detectImpact(uint16_t threshold)
{
    if (!initialized)
        return false;

    uint16_t current = readRaw();
    // Check if the change from baseline exceeds threshold
    return ((current > baseline_value) && ((current - baseline_value) > threshold));
}

void FSR_Sensor::calibrate(uint16_t samples)
{
    if (!initialized)
        return;

    Serial.println("Calibrating FSR - ensure NO pressure on sensor...");
    delay(2000); // Give user time to remove hand

    uint32_t sum = 0;
    Serial.print("Taking ");
    Serial.print(samples);
    Serial.println(" samples...");

    for (uint16_t i = 0; i < samples; i++)
    {
        sum += analogRead(analog_pin);
        delay(5); // Reduced from 10ms - ADC settling time
    }

    baseline_value = sum / samples;
    Serial.print("FSR baseline (averaged over ");
    Serial.print(samples);
    Serial.print(" samples): ");
    Serial.println(baseline_value);
    Serial.println("Calibration complete!");
}

bool FSR_Sensor::isInitialized()
{
    return initialized;
}

void FSR_Sensor::printInfo()
{
    if (!initialized)
    {
        Serial.println("FSR not initialized");
        return;
    }

    Serial.println("=== FSR Sensor Info ===");
    Serial.print("Analog Pin: ");
    Serial.println(analog_pin);
    Serial.print("Baseline value: ");
    Serial.println(baseline_value);
}
