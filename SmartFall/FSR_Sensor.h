#ifndef FSR_SENSOR_H
#define FSR_SENSOR_H

#include <Arduino.h>
#include "Config.h"

class FSR_Sensor
{
private:
    uint8_t analog_pin;
    bool initialized;
    uint16_t baseline_value;

    // Retry configuration for sensor reads
    static const uint8_t MAX_RETRIES = FSR_MAX_RETRIES;
    static const uint16_t RETRY_DELAY_MS = FSR_RETRY_DELAY_MS;

public:
    FSR_Sensor(uint8_t pin = A2);

    bool begin();
    uint16_t readRaw();
    float readForce(); // Approximate force in Newtons
    bool detectImpact(uint16_t threshold = FSR_IMPACT_THRESHOLD);

    void calibrate(uint16_t samples = 100);
    uint16_t getBaseline() const { return baseline_value; }
    bool isInitialized();
    void printInfo();
};

#endif
