#include "MAX30102_Sensor.h"

MAX30102_Sensor::MAX30102_Sensor(uint8_t sda, uint8_t scl, uint8_t rst)
    : initialized(false), sda_pin(sda), scl_pin(scl), rst_pin(rst),
      rateSpot(0), lastBeat(0), beatsPerMinute(0), beatAvg(0) {
    for (byte i = 0; i < RATE_SIZE; i++) {
        rates[i] = 0;
    }
}

bool MAX30102_Sensor::begin() {
    // Configure and perform hardware reset
    pinMode(rst_pin, OUTPUT);
    digitalWrite(rst_pin, HIGH);  // RST is active LOW, so HIGH = normal operation
    delay(100);

    Serial.print("MAX30102: Performing AGGRESSIVE hardware reset on GPIO ");
    Serial.println(rst_pin);

    // First reset attempt
    digitalWrite(rst_pin, LOW);
    delay(100);  // Longer hold time (100ms instead of 10ms)
    digitalWrite(rst_pin, HIGH);
    delay(500);  // Longer wait time (500ms)

    // Second reset attempt (sometimes needed for stubborn modules)
    Serial.println("MAX30102: Second reset attempt...");
    digitalWrite(rst_pin, LOW);
    delay(100);
    digitalWrite(rst_pin, HIGH);
    delay(500);

    Wire.begin(sda_pin, scl_pin);
    delay(100);

    // Try FAST speed first (per Instructables guide for DFRobot modules)
    Serial.println("Attempting library initialization...");
    Serial.println("  Trying I2C_SPEED_FAST (per Instructables guide)...");
    if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) {
        Serial.println("  I2C_SPEED_FAST failed");
        Serial.println("  Trying DEFAULT speed...");
        if (!particleSensor.begin(Wire)) {
            Serial.println("  DEFAULT speed failed");
            Serial.println("  Trying I2C_SPEED_STANDARD...");
            if (!particleSensor.begin(Wire, I2C_SPEED_STANDARD)) {
                Serial.println("Failed to initialize MAX30102 at any speed");
                return false;
            }
        }
    }

    Serial.println("✓ particleSensor.begin() successful!");

    // Additional setup per Instructables guide
    Serial.println("Performing additional sensor setup...");
    particleSensor.setup();  // Configure default settings

    initialized = true;
    return true;
}

void MAX30102_Sensor::configure(byte ledBrightness, byte sampleAverage,
                                 byte ledMode, int sampleRate,
                                 int pulseWidth, int adcRange) {
    if (!initialized) return;

    particleSensor.setup(ledBrightness, sampleAverage, ledMode,
                         sampleRate, pulseWidth, adcRange);

    // Set LED brightness to visible levels
    // Range: 0x00 (off) to 0xFF (max brightness)
    particleSensor.setPulseAmplitudeRed(0xFF);      // Red LED - max brightness
    particleSensor.setPulseAmplitudeIR(0xFF);       // IR LED - max brightness
    particleSensor.setPulseAmplitudeGreen(0);       // Green LED - disabled
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
