#ifndef FSR_SENSOR_H
#define FSR_SENSOR_H

#include "../utils/data_types.h"
#include "../utils/config.h"
#include <Arduino.h>

class FSRSensor {
private:
    uint8_t analog_pin;
    bool initialized;
    uint16_t baseline_value;
    uint16_t impact_threshold;
    uint16_t strap_threshold;
    uint16_t reading_buffer[5];  // Small buffer for impact detection
    uint8_t buffer_index;
    uint32_t last_read_time;
    uint32_t last_impact_time;

public:
    FSRSensor();
    ~FSRSensor();

    // Initialization and configuration
    bool init(uint8_t pin = FSR_ANALOG_PIN);
    bool isInitialized() const;

    // Data reading functions
    uint16_t readRawValue();
    float readPressure();
    bool detectImpact();
    bool isStrapSecure();

    // Calibration functions
    void calibrateBaseline();
    void setImpactThreshold(uint16_t threshold);
    void setStrapThreshold(uint16_t threshold);

    // Utility functions
    bool isDataReady();
    uint16_t getBaseline() const;
    uint32_t getTimeSinceLastImpact();

    // Debug functions
    void printSensorInfo();
    void printRawData();
};

#endif // FSR_SENSOR_H