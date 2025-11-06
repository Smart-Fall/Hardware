#include "MAX30102_Sensor.h"

MAX30102_Sensor::MAX30102_Sensor(uint8_t sda, uint8_t scl, uint8_t rst)
    : initialized(false), sda_pin(sda), scl_pin(scl), rst_pin(rst),
      rateSpot(0), lastBeat(0), beatsPerMinute(0), beatAvg(0) {
    for (byte i = 0; i < RATE_SIZE; i++) {
        rates[i] = 0;
    }
}

bool MAX30102_Sensor::begin() {
    Serial.print("MAX30102::begin() - Initializing Wire on SDA=");
    Serial.print(sda_pin);
    Serial.print(", SCL=");
    Serial.println(scl_pin);

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

    // Test basic I2C read/write
    Serial.println("\nTesting basic I2C communication...");

    // Try to read PART_ID register (0xFF) - should return 0x15 for MAX30102
    Wire.beginTransmission(0x57);
    Wire.write(0xFF);  // PART_ID register
    byte error = Wire.endTransmission();

    Serial.print("  Register read attempt: ");
    if (error == 0) {
        Serial.println("OK");

        // Now read the response
        Wire.requestFrom(0x57, 1);
        if (Wire.available()) {
            byte partID = Wire.read();
            Serial.print("  PART_ID (0xFF): 0x");
            Serial.println(partID, HEX);

            if (partID == 0x15) {
                Serial.println("  ✓ Correct MAX30102 part ID!");
            } else {
                Serial.println("  ⚠ Unexpected part ID (returned 0xFF = sensor not responding properly)");
                Serial.println("  This suggests FIFO overflow or uninitialized state");
            }
        } else {
            Serial.println("  ERROR: No response from sensor");
        }
    } else {
        Serial.print("FAILED (error code: ");
        Serial.print(error);
        Serial.println(")");
    }

    Serial.println("\nAttempting library initialization...");
    Serial.println("  Trying I2C_SPEED_FAST (per Instructables guide)...");

    if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) {
        Serial.println("  I2C_SPEED_FAST failed");
        Serial.println("  Trying DEFAULT speed...");

        if (!particleSensor.begin(Wire)) {
            Serial.println("  DEFAULT speed failed");
            Serial.println("  Trying I2C_SPEED_STANDARD...");

            if (!particleSensor.begin(Wire, I2C_SPEED_STANDARD)) {
                Serial.println("\nFailed to initialize MAX30102 at any speed");
                Serial.println("\nDiagnostic Summary:");
                Serial.println("- Sensor responds at address 0x57: YES");
                Serial.println("- Basic I2C communication: OK");
                Serial.println("- Library initialization: FAILED at ALL speeds");
                Serial.println("\nPossible DFRobot-specific issues:");
                Serial.println("- FIFO buffer overflow/corrupt state (see PART_ID result above)");
                Serial.println("- Module needs complete power cycle (30+ seconds off)");
                Serial.println("- Verify RST pin properly connected to MI (GPIO 21)");
                Serial.println("- Check for configuration jumpers on module");
                Serial.println("\nRecommendation: Disconnect sensor 3.3V for 30+ seconds, then reconnect");
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
