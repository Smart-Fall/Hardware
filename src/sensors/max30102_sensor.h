#ifndef MAX30102_SENSOR_H
#define MAX30102_SENSOR_H

#include "../utils/data_types.h"
#include "../utils/config.h"
#include <Arduino.h>

class MAX30102Sensor {
private:
    bool initialized;
    uint8_t analog_pin;
    float baseline_heart_rate;
    float heart_rate_buffer[10];  // Rolling average buffer
    uint8_t buffer_index;
    uint32_t last_read_time;
    uint32_t last_beat_time;

    // Simulation parameters
    float simulated_bpm;
    float bpm_variation;
    uint32_t simulation_start_time;

public:
    MAX30102Sensor();
    ~MAX30102Sensor();

    // Initialization and configuration
    bool init();
    bool isInitialized() const;

    // Heart rate reading functions
    bool readHeartRate(float& bpm);
    float getAverageHeartRate();
    bool isHeartRateValid();

    // Baseline and calibration
    float getHeartRateBaseline();
    float getHeartRateChange();
    void calibrateBaseline();

    // Simulation control (for testing)
    void setSimulatedHeartRate(float bpm, float variation = 5.0f);
    void simulateStressResponse(float increase_bpm);

    // Utility functions
    bool isDataReady();
    uint32_t getTimeSinceLastBeat();

    // Debug functions
    void printSensorInfo();
    void printRawData();
};

#endif // MAX30102_SENSOR_H