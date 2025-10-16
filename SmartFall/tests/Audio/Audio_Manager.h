#ifndef AUDIO_MANAGER_H
#define AUDIO_MANAGER_H

#include <Arduino.h>

// Alert Patterns
typedef enum {
    ALERT_PATTERN_SINGLE_BEEP,
    ALERT_PATTERN_DOUBLE_BEEP,
    ALERT_PATTERN_TRIPLE_BEEP,
    ALERT_PATTERN_SIREN,
    ALERT_PATTERN_URGENT,
    ALERT_PATTERN_SOS,
    ALERT_PATTERN_CONFIRMATION,
    ALERT_PATTERN_ERROR,
    ALERT_PATTERN_WARNING
} AlertPattern_t;

// Voice-Like Alerts
typedef enum {
    VOICE_ALERT_FALL_DETECTED,
    VOICE_ALERT_PRESS_BUTTON,
    VOICE_ALERT_CALLING_HELP,
    VOICE_ALERT_HELP_SENT,
    VOICE_ALERT_SYSTEM_READY,
    VOICE_ALERT_LOW_BATTERY,
    VOICE_ALERT_CONNECTION_LOST
} VoiceAlert_t;

class Audio_Manager {
private:
    uint8_t speakerPin;
    uint8_t pwmChannel;
    uint8_t volume;  // 0-100
    bool initialized;

    void playToneInternal(uint16_t frequency, uint32_t duration_ms);

public:
    Audio_Manager(uint8_t pin);

    // Initialization
    bool begin();
    bool isInitialized();

    // Volume control
    void setVolume(uint8_t level);  // 0-100
    uint8_t getVolume();

    // Basic tone generation
    void playTone(uint16_t frequency, uint32_t duration_ms);
    void playTones(uint16_t* frequencies, uint32_t* durations, uint8_t count);

    // Alert patterns
    void playPattern(AlertPattern_t pattern, uint8_t repeat = 1);
    void playConfirmationTone();
    void playErrorTone();
    void playWarningTone();
    void playSOSSequence();
    void playStartupMelody();
    void playFallDetectedSequence();

    // Voice-like alerts (using tone patterns)
    void playVoiceAlert(VoiceAlert_t alert);

    // Control
    void stopPattern();
    void silence();
};

#endif // AUDIO_MANAGER_H
