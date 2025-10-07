#ifndef MAX30102_SENSOR_H
#define MAX30102_SENSOR_H

#include <Arduino.h>
#include <Wire.h>
#include <MAX30105.h>
#include <heartRate.h>

class MAX30102_Sensor {
private:
    MAX30105 particleSensor;
    bool initialized;
    uint8_t sda_pin;
    uint8_t scl_pin;

    const byte RATE_SIZE = 4;
    byte rates[4];
    byte rateSpot;
    long lastBeat;
    float beatsPerMinute;
    int beatAvg;

public:
    MAX30102_Sensor(uint8_t sda = 23, uint8_t scl = 22);

    bool begin();
    void configure(byte ledBrightness = 60,
                   byte sampleAverage = 4,
                   byte ledMode = 2,
                   int sampleRate = 100,
                   int pulseWidth = 411,
                   int adcRange = 4096);

    bool readHeartRate(float &bpm, bool &finger_detected);
    long getIRValue();

    bool isInitialized();
    void printInfo();
};

#endif
