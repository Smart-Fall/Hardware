#ifndef MAX30102_SENSOR_H
#define MAX30102_SENSOR_H

#include <Arduino.h>
#include <Wire.h>
#include <DFRobot_BloodOxygen_S.h>

class MAX30102Sensor
{
private:
    DFRobot_BloodOxygen_S_I2C heartRateSensor;
    bool initialized;
    uint8_t sda_pin;
    uint8_t scl_pin;

    // Data buffering for stability
    uint16_t heart_rate_buffer[5];
    uint8_t spo2_buffer[5];
    uint8_t buffer_index;
    uint32_t last_read_time;

    // Baseline values for change detection
    uint16_t baseline_heart_rate;
    uint8_t baseline_spo2;
    bool baseline_set;

public:
    MAX30102Sensor(uint8_t sda = 22, uint8_t scl = 20);

    bool begin(uint8_t address = 0x57);
    void configure();

    bool readData(uint16_t &heart_rate, uint8_t &spo2, float &temperature);
    bool getRawData(uint16_t &heart_rate, uint8_t &spo2);
    float getTemperature();

    void setBaselineHeartRate(uint16_t baseline);
    void setBaselineSPO2(uint8_t baseline);
    uint16_t getBaselineHeartRate();
    uint8_t getBaselineSPO2();

    void resetBaseline();
    void startCollection();
    void stopCollection();

    bool isInitialized();
    void printInfo();
};

#endif
