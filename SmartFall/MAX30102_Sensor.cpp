#include "sensors/MAX30102_Sensor.h"

MAX30102_Sensor::MAX30102_Sensor(uint8_t sda, uint8_t scl)
    : initialized(false), sda_pin(sda), scl_pin(scl),
      rateSpot(0), lastBeat(0), beatsPerMinute(0), beatAvg(0) {
    for (byte i = 0; i < RATE_SIZE; i++) {
        rates[i] = 0;
    }
}

bool MAX30102_Sensor::begin() {
    Wire.begin(sda_pin, scl_pin);

    if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) {
        Serial.println("Failed to initialize MAX30102");
        return false;
    }

    initialized = true;
    return true;
}

void MAX30102_Sensor::configure(byte ledBrightness, byte sampleAverage,
                                 byte ledMode, int sampleRate,
                                 int pulseWidth, int adcRange) {
    if (!initialized) return;

    particleSensor.setup(ledBrightness, sampleAverage, ledMode,
                         sampleRate, pulseWidth, adcRange);
    particleSensor.setPulseAmplitudeRed(0x0A);
    particleSensor.setPulseAmplitudeGreen(0);
}

bool MAX30102_Sensor::readHeartRate(float &bpm, bool &finger_detected) {
    if (!initialized) return false;

    long irValue = particleSensor.getIR();

    finger_detected = (irValue > 50000);

    if (!finger_detected) {
        bpm = 0;
        return false;
    }

    if (checkForBeat(irValue)) {
        long delta = millis() - lastBeat;
        lastBeat = millis();

        beatsPerMinute = 60 / (delta / 1000.0);

        if (beatsPerMinute < 255 && beatsPerMinute > 20) {
            rates[rateSpot++] = (byte)beatsPerMinute;
            rateSpot %= RATE_SIZE;

            beatAvg = 0;
            for (byte x = 0; x < RATE_SIZE; x++) {
                beatAvg += rates[x];
            }
            beatAvg /= RATE_SIZE;
        }
    }

    bpm = beatAvg;
    return (beatAvg > 0);
}

long MAX30102_Sensor::getIRValue() {
    if (!initialized) return 0;
    return particleSensor.getIR();
}

bool MAX30102_Sensor::isInitialized() {
    return initialized;
}

void MAX30102_Sensor::printInfo() {
    if (!initialized) {
        Serial.println("MAX30102 not initialized");
        return;
    }

    Serial.println("=== MAX30102 Info ===");
    Serial.println("Mode: Heart Rate Detection");
    Serial.println("LED: Red + IR");
}
