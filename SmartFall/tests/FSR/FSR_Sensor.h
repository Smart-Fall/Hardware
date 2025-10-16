#ifndef FSR_SENSOR_H
#define FSR_SENSOR_H

#include <Arduino.h>

class FSR_Sensor {
private:
    uint8_t analog_pin;
    bool initialized;
    uint16_t baseline_value;

public:
    FSR_Sensor(uint8_t pin = A2);

    bool begin();
    uint16_t readRaw();
    float readForce();  // Approximate force in Newtons
    bool detectImpact(uint16_t threshold = 500);

    void calibrate();
    bool isInitialized();
    void printInfo();
};

#endif
