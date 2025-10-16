#include "Audio_Manager.h"

Audio_Manager::Audio_Manager(uint8_t pin)
    : speaker_pin(pin), initialized(false), muted(false), volume_level(80),
      playing(false), pattern_start_time(0), current_pattern(ALERT_PATTERN_SINGLE_BEEP),
      pwm_channel(0), pwm_frequency(5000), pwm_resolution(8) {
}

Audio_Manager::~Audio_Manager() {
    end();
}

bool Audio_Manager::begin() {
    if (initialized) {
        Serial.println("[Audio] Already initialized");
        return true;
    }

    // Configure PWM for audio output
    ledcSetup(pwm_channel, pwm_frequency, pwm_resolution);
    ledcAttachPin(speaker_pin, pwm_channel);
    ledcWrite(pwm_channel, 0);  // Start silent

    pinMode(speaker_pin, OUTPUT);
    digitalWrite(speaker_pin, LOW);

    initialized = true;
    Serial.println("[Audio] PAM8302 amplifier initialized");

    return true;
}

void Audio_Manager::end() {
    if (initialized) {
        stopTone();
        ledcDetachPin(speaker_pin);
        initialized = false;
        Serial.println("[Audio] Audio system stopped");
    }
}

void Audio_Manager::setVolume(uint8_t level) {
    volume_level = constrain(level, 0, 100);
    if (DEBUG_COMMUNICATION) {
        Serial.print("[Audio] Volume set to: ");
        Serial.print(volume_level);
        Serial.println("%");
    }
}

uint8_t Audio_Manager::getVolume() {
    return volume_level;
}

void Audio_Manager::mute() {
    muted = true;
    if (playing) {
        toneOff();
    }
    if (DEBUG_COMMUNICATION) {
        Serial.println("[Audio] Muted");
    }
}

void Audio_Manager::unmute() {
    muted = false;
    if (DEBUG_COMMUNICATION) {
        Serial.println("[Audio] Unmuted");
    }
}

bool Audio_Manager::isMuted() {
    return muted;
}

void Audio_Manager::playTone(uint16_t frequency, uint32_t duration_ms) {
    playTone(frequency, duration_ms, volume_level);
}

void Audio_Manager::playTone(uint16_t frequency, uint32_t duration_ms, uint8_t volume) {
    if (!initialized || muted) return;

    toneOn(frequency, volume);
    delay(duration_ms);
    toneOff();
}

void Audio_Manager::stopTone() {
    toneOff();
    playing = false;
}

void Audio_Manager::playPattern(AlertPattern_t pattern) {
    playPattern(pattern, 1);
}

void Audio_Manager::playPattern(AlertPattern_t pattern, uint8_t repetitions) {
    if (!initialized || muted) return;

    current_pattern = pattern;
    playing = true;

    for (uint8_t i = 0; i < repetitions; i++) {
        switch (pattern) {
            case ALERT_PATTERN_SINGLE_BEEP:
                playSingleBeep();
                break;

            case ALERT_PATTERN_DOUBLE_BEEP:
                playDoubleBeep();
                break;

            case ALERT_PATTERN_TRIPLE_BEEP:
                playTripleBeep();
                break;

            case ALERT_PATTERN_CONTINUOUS:
                playContinuousTone();
                break;

            case ALERT_PATTERN_SIREN:
                playSiren();
                break;

            case ALERT_PATTERN_URGENT:
                playUrgentBeeps();
                break;

            case ALERT_PATTERN_CONFIRMED:
                playConfirmationTone();
                break;

            case ALERT_PATTERN_ERROR:
                playErrorTone();
                break;

            case ALERT_PATTERN_STARTUP:
                playStartupMelody();
                break;

            case ALERT_PATTERN_FALL_DETECTED:
                playFallDetectedSequence();
                break;

            case ALERT_PATTERN_SOS:
                playSOSSequence();
                break;

            case ALERT_PATTERN_CANCEL:
                playFallingTone(1000, 500, 300);
                break;
        }

        if (i < repetitions - 1) {
            delay(500);  // Pause between repetitions
        }
    }

    playing = false;
}

void Audio_Manager::stopPattern() {
    playing = false;
    stopTone();
}

bool Audio_Manager::isPlaying() {
    return playing;
}

void Audio_Manager::playVoiceAlert(VoiceAlert_t alert) {
    playVoiceAlert(alert, 1);
}

void Audio_Manager::playVoiceAlert(VoiceAlert_t alert, uint8_t repetitions) {
    if (!initialized || muted) return;

    playing = true;

    for (uint8_t i = 0; i < repetitions; i++) {
        switch (alert) {
            case VOICE_ALERT_FALL_DETECTED:
                // "Fall" (falling tone) + "Detected" (rising tone)
                playFallingTone(800, 400, 300);
                delay(100);
                playRisingTone(400, 800, 300);
                delay(200);
                playShortTone(1000, 200);  // Emphasis
                break;

            case VOICE_ALERT_PRESS_BUTTON:
                // "Press" + "Button" + "If" + "Okay"
                playShortTone(600, 150);
                delay(80);
                playShortTone(700, 150);
                delay(150);
                playShortTone(500, 100);
                delay(80);
                playLongTone(600, 250);
                break;

            case VOICE_ALERT_CALLING_HELP:
                // "Calling" (ascending) + "Help" (urgent)
                playRisingTone(500, 1000, 400);
                delay(150);
                playShortTone(1500, 200);
                delay(100);
                playShortTone(1500, 200);
                break;

            case VOICE_ALERT_HELP_SENT:
                // "Help" + "Sent" (confirmation)
                playShortTone(800, 150);
                delay(100);
                playRisingTone(800, 1200, 250);
                break;

            case VOICE_ALERT_SYSTEM_READY:
                // "System" + "Ready" (positive ascending)
                playShortTone(600, 150);
                delay(80);
                playShortTone(700, 150);
                delay(150);
                playRisingTone(700, 1000, 300);
                break;

            case VOICE_ALERT_LOW_BATTERY:
                // "Low" (descending) + "Battery" (repeated warning)
                playFallingTone(800, 400, 300);
                delay(150);
                playShortTone(500, 150);
                delay(100);
                playShortTone(500, 150);
                delay(100);
                playShortTone(500, 150);
                break;

            case VOICE_ALERT_CONNECTION_LOST:
                // "Connection" + "Lost" (falling)
                playShortTone(700, 150);
                delay(80);
                playShortTone(650, 150);
                delay(80);
                playFallingTone(600, 300, 400);
                break;

            case VOICE_ALERT_COUNTDOWN:
                playCountdownBeeps(5);
                break;
        }

        if (i < repetitions - 1) {
            delay(800);  // Pause between repetitions
        }
    }

    playing = false;
}

void Audio_Manager::playStartupMelody() {
    // Ascending scale to indicate system ready
    playShortTone(523, 150);  // C
    delay(50);
    playShortTone(659, 150);  // E
    delay(50);
    playShortTone(784, 150);  // G
    delay(50);
    playLongTone(1047, 300);  // C (octave higher)
}

void Audio_Manager::playConfirmationTone() {
    // Ascending two-note confirmation
    playShortTone(800, 100);
    delay(50);
    playShortTone(1200, 200);
}

void Audio_Manager::playErrorTone() {
    // Descending two-note error
    playShortTone(800, 150);
    delay(50);
    playShortTone(400, 300);
}

void Audio_Manager::playWarningTone() {
    // Alternating warning beeps
    for (int i = 0; i < 3; i++) {
        playShortTone(1000, 150);
        delay(100);
    }
}

void Audio_Manager::playSirenSound() {
    // Alternating high/low siren
    for (int i = 0; i < 3; i++) {
        playSweep(800, 1500, 500, 1);   // Rising
        playSweep(1500, 800, 500, -1);  // Falling
    }
}

void Audio_Manager::playFallDetectedSequence() {
    // Urgent three-tone sequence
    playShortTone(1500, 200);
    delay(150);
    playShortTone(1500, 200);
    delay(150);
    playLongTone(1500, 400);
    delay(300);

    // Voice-like "Fall Detected"
    playVoiceAlert(VOICE_ALERT_FALL_DETECTED);
}

void Audio_Manager::playSOSSequence() {
    // SOS in Morse code: ... --- ...
    // Three short beeps
    for (int i = 0; i < 3; i++) {
        playShortTone(1500, 150);
        delay(150);
    }
    delay(300);

    // Three long beeps
    for (int i = 0; i < 3; i++) {
        playLongTone(1500, 400);
        delay(150);
    }
    delay(300);

    // Three short beeps
    for (int i = 0; i < 3; i++) {
        playShortTone(1500, 150);
        delay(150);
    }
}

void Audio_Manager::playCountdownBeeps(uint8_t count) {
    for (uint8_t i = 0; i < count; i++) {
        // Higher pitch for final beep
        uint16_t freq = (i == count - 1) ? 1500 : 1000;
        playShortTone(freq, 200);
        delay(800);  // 1 second between beeps
    }
}

bool Audio_Manager::isInitialized() {
    return initialized;
}

void Audio_Manager::test() {
    if (!initialized || muted) return;

    Serial.println("[Audio] Testing all patterns...");

    Serial.println("  - Single Beep");
    playPattern(ALERT_PATTERN_SINGLE_BEEP);
    delay(500);

    Serial.println("  - Double Beep");
    playPattern(ALERT_PATTERN_DOUBLE_BEEP);
    delay(500);

    Serial.println("  - Triple Beep");
    playPattern(ALERT_PATTERN_TRIPLE_BEEP);
    delay(500);

    Serial.println("  - Confirmation Tone");
    playConfirmationTone();
    delay(500);

    Serial.println("  - Error Tone");
    playErrorTone();
    delay(500);

    Serial.println("  - Startup Melody");
    playStartupMelody();
    delay(500);

    Serial.println("  - Fall Detected Sequence");
    playFallDetectedSequence();
    delay(1000);

    Serial.println("  - SOS Sequence");
    playSOSSequence();
    delay(1000);

    Serial.println("[Audio] Test complete");
}

// Private helper functions

void Audio_Manager::toneOn(uint16_t frequency) {
    toneOn(frequency, volume_level);
}

void Audio_Manager::toneOn(uint16_t frequency, uint8_t volume) {
    if (!initialized || muted) return;

    // Use ESP32 LED PWM for tone generation
    ledcSetup(pwm_channel, frequency, pwm_resolution);
    uint8_t duty = scaleVolume(volume);
    ledcWrite(pwm_channel, duty);
}

void Audio_Manager::toneOff() {
    if (!initialized) return;
    ledcWrite(pwm_channel, 0);
}

void Audio_Manager::playSingleBeep() {
    playShortTone(TONE_MEDIUM_FREQ, 200);
}

void Audio_Manager::playDoubleBeep() {
    playShortTone(TONE_MEDIUM_FREQ, 150);
    delay(150);
    playShortTone(TONE_MEDIUM_FREQ, 150);
}

void Audio_Manager::playTripleBeep() {
    for (int i = 0; i < 3; i++) {
        playShortTone(TONE_MEDIUM_FREQ, 150);
        delay(150);
    }
}

void Audio_Manager::playContinuousTone() {
    playLongTone(TONE_HIGH_FREQ, 2000);
}

void Audio_Manager::playSiren() {
    for (int i = 0; i < 5; i++) {
        playSweep(600, 1200, 300, 1);
        playSweep(1200, 600, 300, -1);
    }
}

void Audio_Manager::playUrgentBeeps() {
    for (int i = 0; i < 5; i++) {
        playShortTone(TONE_URGENT_FREQ, 100);
        delay(100);
    }
}

void Audio_Manager::playShortTone(uint16_t freq, uint32_t duration) {
    playTone(freq, duration);
}

void Audio_Manager::playLongTone(uint16_t freq, uint32_t duration) {
    playTone(freq, duration);
}

void Audio_Manager::playRisingTone(uint16_t start_freq, uint16_t end_freq, uint32_t duration) {
    playSweep(start_freq, end_freq, duration, 1);
}

void Audio_Manager::playFallingTone(uint16_t start_freq, uint16_t end_freq, uint32_t duration) {
    playSweep(start_freq, end_freq, duration, -1);
}

void Audio_Manager::playSweep(uint16_t start_freq, uint16_t end_freq, uint32_t duration, int8_t direction) {
    if (!initialized || muted) return;

    uint32_t start_time = millis();
    uint32_t step_time = 10;  // Update frequency every 10ms

    while (millis() - start_time < duration) {
        float progress = (float)(millis() - start_time) / duration;
        uint16_t current_freq = start_freq + (end_freq - start_freq) * progress;

        toneOn(current_freq);
        delay(step_time);
    }

    toneOff();
}

void Audio_Manager::delayWithStop(uint32_t ms) {
    uint32_t start = millis();
    while (millis() - start < ms) {
        if (!playing) {
            break;
        }
        delay(10);
    }
}

uint8_t Audio_Manager::scaleVolume(uint8_t volume) {
    // Convert 0-100 volume to PWM duty cycle (0-255)
    // Apply logarithmic scaling for more natural volume curve
    float normalized = volume / 100.0;
    float scaled = pow(normalized, 2.0);  // Square for logarithmic feel
    return (uint8_t)(scaled * 255);
}
