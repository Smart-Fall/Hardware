#ifndef MAX30102_SENSOR_H
#define MAX30102_SENSOR_H

#include <Arduino.h>
#include <Wire.h>
#include <HardwareSerial.h>
#include <DFRobot_BloodOxygen_S.h>
#include "Board_Config.h"
#include "Config.h"

class MAX30102Sensor
{
private:
    #ifdef MAX30102_USE_UART
        DFRobot_BloodOxygen_S_HardWareUart heartRateSensor;
        uint8_t rx_pin;
        uint8_t tx_pin;
    #else
        DFRobot_BloodOxygen_S_I2C heartRateSensor;
        uint8_t sda_pin;
        uint8_t scl_pin;
    #endif
    bool initialized;

    // Data buffering for stability
    uint16_t heart_rate_buffer[5];
    uint8_t spo2_buffer[5];
    uint8_t buffer_index;
    uint32_t last_read_time;

    // Baseline values for change detection
    uint16_t baseline_heart_rate;
    uint8_t baseline_spo2;
    bool baseline_set;

    // Retry configuration for communication
    static const uint8_t MAX_RETRIES = I2C_SENSOR_MAX_RETRIES;
    static const uint16_t RETRY_DELAY_MS = I2C_SENSOR_RETRY_DELAY_MS;

    // Stale data detection
    uint16_t last_heart_rate = 0;
    uint8_t stale_count = 0;
    static const uint8_t STALE_THRESHOLD = SENSOR_STALE_THRESHOLD;

public:
    #ifdef MAX30102_USE_UART
        MAX30102Sensor(uint8_t rx = 255, uint8_t tx = 255);
    #else
        MAX30102Sensor(uint8_t sda = 255, uint8_t scl = 255);
    #endif

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
