#ifndef FSR_H
#define FSR_H

#include <Arduino.h>

class FSRSensor
{
private:
    uint8_t analog_pin;
    bool initialized;
    uint16_t baseline_value;

public:
    FSRSensor(uint8_t pin = A2);

    bool begin();
    uint16_t readRaw();
    float readForce(); // Approximate force in Newtons
    bool detectImpact(uint16_t threshold = 500);

    void calibrate(uint16_t samples = 100);
    bool isInitialized();
    void printInfo();
};

#endif
