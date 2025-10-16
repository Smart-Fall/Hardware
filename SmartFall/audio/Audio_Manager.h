#ifndef AUDIO_MANAGER_H
#define AUDIO_MANAGER_H

#include <Arduino.h>
#include "../utils/config.h"
#include "../utils/data_types.h"

// Tone frequencies (in Hz)
#define TONE_LOW_FREQ       200
#define TONE_MEDIUM_FREQ    500
#define TONE_HIGH_FREQ      1000
#define TONE_URGENT_FREQ    1500
#define TONE_ALERT_FREQ     2000

// Alert pattern types
typedef enum {
    ALERT_PATTERN_SINGLE_BEEP,      // Single beep
    ALERT_PATTERN_DOUBLE_BEEP,      // Two short beeps
    ALERT_PATTERN_TRIPLE_BEEP,      // Three short beeps
    ALERT_PATTERN_CONTINUOUS,       // Continuous tone
    ALERT_PATTERN_SIREN,            // Alternating high/low siren
    ALERT_PATTERN_URGENT,           // Fast repeating beeps
    ALERT_PATTERN_CONFIRMED,        // Confirmation tone (ascending)
    ALERT_PATTERN_ERROR,            // Error tone (descending)
    ALERT_PATTERN_STARTUP,          // Startup melody
    ALERT_PATTERN_FALL_DETECTED,    // Fall detection sequence
    ALERT_PATTERN_SOS,              // SOS emergency sequence
    ALERT_PATTERN_CANCEL            // Alert cancelled sequence
} AlertPattern_t;

// Voice-like alert types
typedef enum {
    VOICE_ALERT_FALL_DETECTED,      // "Fall detected" alert
    VOICE_ALERT_PRESS_BUTTON,       // "Press button if okay"
    VOICE_ALERT_CALLING_HELP,       // "Calling for help"
    VOICE_ALERT_HELP_SENT,          // "Help notification sent"
    VOICE_ALERT_SYSTEM_READY,       // "System ready"
    VOICE_ALERT_LOW_BATTERY,        // "Low battery"
    VOICE_ALERT_CONNECTION_LOST,    // "Connection lost"
    VOICE_ALERT_COUNTDOWN           // Countdown beeps
} VoiceAlert_t;

class Audio_Manager {
private:
    uint8_t speaker_pin;
    bool initialized;
    bool muted;
    uint8_t volume_level;  // 0-100 (controls duty cycle for PWM)

    // Current playback state
    bool playing;
    uint32_t pattern_start_time;
    AlertPattern_t current_pattern;

    // PWM configuration
    uint8_t pwm_channel;
    uint32_t pwm_frequency;
    uint8_t pwm_resolution;  // bits

public:
    Audio_Manager(uint8_t pin = SPEAKER_PIN);
    ~Audio_Manager();

    // Initialization
    bool begin();
    void end();

    // Volume control
    void setVolume(uint8_t level);  // 0-100
    uint8_t getVolume();
    void mute();
    void unmute();
    bool isMuted();

    // Basic tone generation
    void playTone(uint16_t frequency, uint32_t duration_ms);
    void playTone(uint16_t frequency, uint32_t duration_ms, uint8_t volume);
    void stopTone();

    // Alert patterns
    void playPattern(AlertPattern_t pattern);
    void playPattern(AlertPattern_t pattern, uint8_t repetitions);
    void stopPattern();
    bool isPlaying();

    // Voice-like alerts (tone sequences that mimic speech patterns)
    void playVoiceAlert(VoiceAlert_t alert);
    void playVoiceAlert(VoiceAlert_t alert, uint8_t repetitions);

    // Pre-defined melodies
    void playStartupMelody();
    void playConfirmationTone();
    void playErrorTone();
    void playWarningTone();
    void playSirenSound();

    // Emergency sequences
    void playFallDetectedSequence();
    void playSOSSequence();
    void playCountdownBeeps(uint8_t count);

    // Utility functions
    bool isInitialized();
    void test();  // Test all patterns

private:
    // Internal tone generation
    void toneOn(uint16_t frequency);
    void toneOn(uint16_t frequency, uint8_t volume);
    void toneOff();

    // Pattern implementations
    void playSingleBeep();
    void playDoubleBeep();
    void playTripleBeep();
    void playContinuousTone();
    void playSiren();
    void playUrgentBeeps();

    // Voice-like pattern helpers
    void playShortTone(uint16_t freq, uint32_t duration);
    void playLongTone(uint16_t freq, uint32_t duration);
    void playRisingTone(uint16_t start_freq, uint16_t end_freq, uint32_t duration);
    void playFallingTone(uint16_t start_freq, uint16_t end_freq, uint32_t duration);
    void playSweep(uint16_t start_freq, uint16_t end_freq, uint32_t duration, int8_t direction);

    // Helper functions
    void delayWithStop(uint32_t ms);  // Delay that can be interrupted
    uint8_t scaleVolume(uint8_t volume);
};

#endif // AUDIO_MANAGER_H
