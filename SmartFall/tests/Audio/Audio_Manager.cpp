#include "Audio_Manager.h"

Audio_Manager::Audio_Manager(uint8_t pin) {
    speakerPin = pin;
    pwmChannel = 0;  // Use PWM channel 0
    volume = 80;     // Default 80%
    initialized = false;
}

bool Audio_Manager::begin() {
    // Configure PWM for audio output
    ledcSetup(pwmChannel, 5000, 8);  // 5kHz, 8-bit resolution
    ledcAttachPin(speakerPin, pwmChannel);
    ledcWrite(pwmChannel, 0);  // Start silent

    initialized = true;
    Serial.println("Audio Manager initialized (PAM8302)");
    return true;
}

bool Audio_Manager::isInitialized() {
    return initialized;
}

void Audio_Manager::setVolume(uint8_t level) {
    volume = constrain(level, 0, 100);
    Serial.print("Volume set to: ");
    Serial.print(volume);
    Serial.println("%");
}

uint8_t Audio_Manager::getVolume() {
    return volume;
}

void Audio_Manager::playToneInternal(uint16_t frequency, uint32_t duration_ms) {
    if (!initialized || frequency == 0) return;

    // Calculate duty cycle based on volume (0-255 for 8-bit)
    uint8_t dutyCycle = map(volume, 0, 100, 0, 128);  // Max 50% duty cycle for square wave

    ledcWriteTone(pwmChannel, frequency);
    ledcWrite(pwmChannel, dutyCycle);
    delay(duration_ms);
    ledcWrite(pwmChannel, 0);  // Silence
}

void Audio_Manager::playTone(uint16_t frequency, uint32_t duration_ms) {
    playToneInternal(frequency, duration_ms);
}

void Audio_Manager::playTones(uint16_t* frequencies, uint32_t* durations, uint8_t count) {
    for (uint8_t i = 0; i < count; i++) {
        playToneInternal(frequencies[i], durations[i]);
        delay(50);  // Small gap between tones
    }
}

void Audio_Manager::playPattern(AlertPattern_t pattern, uint8_t repeat) {
    for (uint8_t r = 0; r < repeat; r++) {
        switch (pattern) {
            case ALERT_PATTERN_SINGLE_BEEP:
                playToneInternal(1000, 300);
                break;

            case ALERT_PATTERN_DOUBLE_BEEP:
                playToneInternal(1000, 200);
                delay(100);
                playToneInternal(1000, 200);
                break;

            case ALERT_PATTERN_TRIPLE_BEEP:
                playToneInternal(1000, 150);
                delay(100);
                playToneInternal(1000, 150);
                delay(100);
                playToneInternal(1000, 150);
                break;

            case ALERT_PATTERN_SIREN:
                for (uint16_t freq = 500; freq <= 2000; freq += 50) {
                    playToneInternal(freq, 30);
                }
                for (uint16_t freq = 2000; freq >= 500; freq -= 50) {
                    playToneInternal(freq, 30);
                }
                break;

            case ALERT_PATTERN_URGENT:
                for (int i = 0; i < 5; i++) {
                    playToneInternal(2000, 100);
                    delay(50);
                }
                break;

            case ALERT_PATTERN_SOS:
                playSOSSequence();
                break;

            case ALERT_PATTERN_CONFIRMATION:
                playConfirmationTone();
                break;

            case ALERT_PATTERN_ERROR:
                playErrorTone();
                break;

            case ALERT_PATTERN_WARNING:
                playWarningTone();
                break;
        }

        if (r < repeat - 1) {
            delay(500);  // Delay between repetitions
        }
    }
}

void Audio_Manager::playConfirmationTone() {
    playToneInternal(1000, 100);
    delay(50);
    playToneInternal(1500, 100);
}

void Audio_Manager::playErrorTone() {
    playToneInternal(400, 200);
    delay(100);
    playToneInternal(300, 300);
}

void Audio_Manager::playWarningTone() {
    for (int i = 0; i < 3; i++) {
        playToneInternal(800, 200);
        delay(100);
    }
}

void Audio_Manager::playSOSSequence() {
    // S: ... (3 short)
    for (int i = 0; i < 3; i++) {
        playToneInternal(1000, 200);
        delay(100);
    }
    delay(300);

    // O: --- (3 long)
    for (int i = 0; i < 3; i++) {
        playToneInternal(1000, 600);
        delay(100);
    }
    delay(300);

    // S: ... (3 short)
    for (int i = 0; i < 3; i++) {
        playToneInternal(1000, 200);
        delay(100);
    }
}

void Audio_Manager::playStartupMelody() {
    uint16_t frequencies[] = {523, 587, 659, 698, 784};  // C, D, E, F, G
    for (int i = 0; i < 5; i++) {
        playToneInternal(frequencies[i], 150);
        delay(50);
    }
}

void Audio_Manager::playFallDetectedSequence() {
    // Urgent ascending pattern
    for (int i = 0; i < 3; i++) {
        playToneInternal(800, 200);
        playToneInternal(1200, 200);
        playToneInternal(1600, 200);
        delay(200);
    }
}

void Audio_Manager::playVoiceAlert(VoiceAlert_t alert) {
    // Voice-like patterns using tone variations
    // These patterns mimic speech prosody with varying frequencies and durations

    switch (alert) {
        case VOICE_ALERT_FALL_DETECTED:
            // "Fall Detected" - 2 words
            // "Fall" - descending
            playToneInternal(800, 150);
            playToneInternal(600, 200);
            delay(100);
            // "De-tec-ted" - 3 syllables
            playToneInternal(700, 120);
            playToneInternal(900, 120);
            playToneInternal(600, 180);
            break;

        case VOICE_ALERT_PRESS_BUTTON:
            // "Press Button" - 2 words
            // "Press" - rising
            playToneInternal(600, 180);
            delay(80);
            // "But-ton" - 2 syllables
            playToneInternal(700, 150);
            playToneInternal(650, 180);
            break;

        case VOICE_ALERT_CALLING_HELP:
            // "Calling Help" - 2 words
            // "Call-ing" - 2 syllables
            playToneInternal(750, 150);
            playToneInternal(700, 150);
            delay(100);
            // "Help" - emphasized
            playToneInternal(900, 250);
            break;

        case VOICE_ALERT_HELP_SENT:
            // "Help Sent" - 2 words
            // "Help" - rising
            playToneInternal(700, 180);
            delay(80);
            // "Sent" - falling
            playToneInternal(800, 120);
            playToneInternal(650, 180);
            break;

        case VOICE_ALERT_SYSTEM_READY:
            // "System Ready" - 2 words
            // "Sys-tem" - 2 syllables
            playToneInternal(650, 120);
            playToneInternal(700, 120);
            delay(100);
            // "Read-y" - 2 syllables, rising
            playToneInternal(750, 150);
            playToneInternal(850, 180);
            break;

        case VOICE_ALERT_LOW_BATTERY:
            // "Low Battery" - 3 syllables
            // "Low"
            playToneInternal(600, 180);
            delay(80);
            // "Bat-ter-y" - 3 syllables
            playToneInternal(650, 120);
            playToneInternal(700, 120);
            playToneInternal(550, 200);
            break;

        case VOICE_ALERT_CONNECTION_LOST:
            // "Connection Lost" - 4 syllables
            // "Con-nec-tion"
            playToneInternal(700, 100);
            playToneInternal(750, 100);
            playToneInternal(700, 120);
            delay(80);
            // "Lost" - falling
            playToneInternal(650, 120);
            playToneInternal(500, 180);
            break;
    }
}

void Audio_Manager::stopPattern() {
    ledcWrite(pwmChannel, 0);
}

void Audio_Manager::silence() {
    ledcWrite(pwmChannel, 0);
}
