# Audio System

Comprehensive guide to the SmartFall audio alert system powered by the PAM8302 amplifier.

## Hardware Overview

| Component | Specification | Details |
|-----------|---------------|---------|
| **Amplifier** | PAM8302A | 2.5W Class D mono amplifier |
| **Output** | Stereo/Mono | Configurable |
| **Frequency Response** | 20Hz - 20kHz | Audio range |
| **THD** | <1% | Clean audio |
| **Power Supply** | 3.3V | From ESP32 regulator |

### Wiring

```
ESP32 GPIO 25 (PWM 5kHz) ──→ PAM8302 A+ (Audio Input+)
ESP32 GND                 ──→ PAM8302 A- (Audio Input-)
ESP32 3.3V                ──→ PAM8302 VIN (Power)
ESP32 GND                 ──→ PAM8302 GND (Ground)
PAM8302 OUT+              ──→ Speaker(+)
PAM8302 OUT-              ──→ Speaker(-)/GND
```

## Audio Manager API

### Initialization

```cpp
#include "Audio_Manager.h"

AudioManager audio_manager;

// Initialize audio system
bool success = audio_manager.begin();

// Set default volume (0-100%)
audio_manager.setVolume(80);

// Get current volume
uint8_t volume = audio_manager.getVolume();
```

### Configuration (Config.h)

```cpp
// Audio Configuration
#define AUDIO_DEFAULT_VOLUME       80      // 0-100
#define AUDIO_PWM_CHANNEL          0       // ESP32 PWM channel
#define AUDIO_PWM_FREQUENCY        5000    // Base frequency (Hz)
#define AUDIO_PWM_RESOLUTION       8       // 8-bit resolution (0-255)
#define AUDIO_ENABLE_VOICE_ALERTS  true    // Enable voice patterns
```

## Alert Patterns

### Single Patterns

```cpp
// Single beep
audio_manager.playPattern(ALERT_PATTERN_SINGLE_BEEP);

// Double beep
audio_manager.playPattern(ALERT_PATTERN_DOUBLE_BEEP);

// Triple beep
audio_manager.playPattern(ALERT_PATTERN_TRIPLE_BEEP);
```

### Repeating Patterns

```cpp
// Play siren 3 times
audio_manager.playPattern(ALERT_PATTERN_SIREN, 3);

// Play urgent pattern 2 times
audio_manager.playPattern(ALERT_PATTERN_URGENT, 2);

// Available patterns:
// - ALERT_PATTERN_SINGLE_BEEP      (100ms)
// - ALERT_PATTERN_DOUBLE_BEEP      (100ms × 2 + 100ms gap)
// - ALERT_PATTERN_TRIPLE_BEEP      (100ms × 3)
// - ALERT_PATTERN_SIREN            (500ms tone sweep)
// - ALERT_PATTERN_URGENT           (alternating frequencies)
// - ALERT_PATTERN_SOS              (Morse: ... --- ...)
// - ALERT_PATTERN_CONFIRMATION     (ascending tone)
// - ALERT_PATTERN_ERROR            (descending tone)
// - ALERT_PATTERN_WARNING          (alternating tone)
```

### Morse Code SOS

```cpp
// SOS sequence (Morse code: ... --- ...)
audio_manager.playPattern(ALERT_PATTERN_SOS);

// Timing:
// Dot:   100ms tone, 100ms silence
// Dash:  300ms tone, 100ms silence
// Between letters: 300ms silence
// SOS sequence: ~1.9 seconds total
```

## Voice-Like Alerts

Sequences of tones designed to convey meaning:

```cpp
// Fall detected announcement
audio_manager.playVoiceAlert(VOICE_ALERT_FALL_DETECTED);

// User guidance
audio_manager.playVoiceAlert(VOICE_ALERT_PRESS_BUTTON);

// Status notifications
audio_manager.playVoiceAlert(VOICE_ALERT_SYSTEM_READY);
audio_manager.playVoiceAlert(VOICE_ALERT_CALLING_HELP);
audio_manager.playVoiceAlert(VOICE_ALERT_HELP_SENT);

// Warnings
audio_manager.playVoiceAlert(VOICE_ALERT_LOW_BATTERY);
audio_manager.playVoiceAlert(VOICE_ALERT_CONNECTION_LOST);

// Available voice alerts:
// VOICE_ALERT_SYSTEM_READY        (startup tone)
// VOICE_ALERT_FALL_DETECTED       (alert sequence)
// VOICE_ALERT_PRESS_BUTTON        (user prompt)
// VOICE_ALERT_CALLING_HELP        (emergency sequence)
// VOICE_ALERT_HELP_SENT           (confirmation tone)
// VOICE_ALERT_LOW_BATTERY         (warning sequence)
// VOICE_ALERT_CONNECTION_LOST     (error tone)
```

## Specialized Sequences

```cpp
// Fall detected with siren (5 seconds)
audio_manager.playFallDetectedSequence();

// SOS in Morse code (... --- ...)
audio_manager.playSOSSequence();

// System feedback tones
audio_manager.playConfirmationTone();  // Ascending success tone
audio_manager.playErrorTone();         // Descending error tone
audio_manager.playWarningTone();       // Alternating warning

// Startup melody
audio_manager.playStartupMelody();

// Stop all audio
audio_manager.stop();
```

## Volume Control

### Setting Volume

```cpp
// Set volume 0-100%
audio_manager.setVolume(50);    // Quiet
audio_manager.setVolume(80);    // Normal (default)
audio_manager.setVolume(100);   // Maximum

// Volume affects all subsequent audio
// Only applies to playback, not to pre-generated patterns
```

### Power Consumption vs Volume

| Volume | Current | Power | Notes |
|--------|---------|-------|-------|
| **Idle** | 50 mA | 165 mW | No audio |
| **10%** | 60 mA | 198 mW | Very quiet |
| **25%** | 80 mA | 264 mW | Quiet |
| **50%** | 150 mA | 495 mW | Normal conversation level |
| **75%** | 250 mA | 825 mW | Loud |
| **100%** | 300 mA | 990 mW | Maximum (ear protection!) |

## Frequency Reference

### Standard Tones Used

| Alert Type | Frequency | Duration | Purpose |
|------------|-----------|----------|---------|
| **Alert Beep** | 800 Hz | 100 ms | Generic alert |
| **Warning Siren** | 400-1200 Hz sweep | 500 ms | Emergency warning |
| **Confirmation** | 200 Hz → 800 Hz | 500 ms | Success indicator |
| **Error** | 800 Hz → 200 Hz | 500 ms | Error indicator |
| **SOS Morse** | 700 Hz | Variable | International distress |

## Integration with Fall Detection

### Emergency Alert Sequence

```cpp
// In main loop when fall detected
if (fall_detector.getCurrentStatus() >= CONFIRMED_FALL) {
    // Immediate audio alert
    audio_manager.setVolume(100);
    audio_manager.playFallDetectedSequence();

    // Send communication alerts
    emergency_comms.sendAlert(emergency_data);

    // Optional: escalating alerts
    delay(5000);  // Wait 5 seconds
    if (still_alert_active) {
        audio_manager.playPattern(ALERT_PATTERN_URGENT, 3);
    }
}
```

### Cancellation Alert

```cpp
// If user cancels alert (movement detected)
audio_manager.setVolume(60);
audio_manager.playPattern(ALERT_PATTERN_DOUBLE_BEEP);
Serial.println("Alert cancelled - movement detected");
```

### Status Feedback

```cpp
// WiFi connected
audio_manager.playVoiceAlert(VOICE_ALERT_SYSTEM_READY);

// Low battery warning
if (battery_level < 20) {
    audio_manager.playVoiceAlert(VOICE_ALERT_LOW_BATTERY);
}

// Connection lost
if (!wifi_manager.isConnected() && !ble_server.isConnected()) {
    audio_manager.playVoiceAlert(VOICE_ALERT_CONNECTION_LOST);
}
```

## Advanced Features

### Custom Tone Generation

```cpp
// Play custom frequency for duration
audio_manager.playTone(800,   // Frequency (Hz)
                      1000,   // Duration (ms)
                      80);    // Volume (0-100)

// Frequency sweep (siren effect)
audio_manager.playSweep(400,   // Start frequency (Hz)
                       1200,   // End frequency (Hz)
                        500,   // Duration (ms)
                         80);  // Volume (0-100)
```

### Non-Blocking Playback

```cpp
// Check if currently playing
if (audio_manager.isPlaying()) {
    Serial.println("Audio is currently playing");
} else {
    // Safe to start new audio
    audio_manager.playPattern(ALERT_PATTERN_SIREN);
}

// Get current playback position
uint32_t position = audio_manager.getPlaybackPosition();
```

### Audio Testing

Use the Audio test sketch in `SmartFall/tests/Audio/`:

```bash
cd SmartFall/tests/Audio
arduino-cli compile --fqbn esp32:esp32:adafruit_feather_esp32_v2 .
arduino-cli upload -p PORT --fqbn esp32:esp32:adafruit_feather_esp32_v2 .
```

The test will automatically play:
1. Startup melody
2. Volume levels (25%, 50%, 75%, 100%)
3. Frequency sweep (500Hz - 2000Hz)
4. All alert patterns
5. SOS Morse code
6. Voice-like alert sequences

### Serial Debug Output

```cpp
// Enable audio debugging
#define DEBUG_AUDIO true

// Output example:
// Audio: Playing ALERT_PATTERN_SIREN (500ms duration)
// Audio: Frequency: 400-1200 Hz sweep
// Audio: Volume: 80%
// Audio: Playback complete
```

## Troubleshooting

### No Audio Output

1. **Check speaker connection**
   - Verify speaker wired to PAM8302 OUT+/OUT-
   - Test with another audio source

2. **Check amplifier power**
   - Verify 3.3V on PAM8302 VIN
   - Check ground connection

3. **Check GPIO 25**
   - Verify GPIO 25 PWM signal with oscilloscope (5 kHz)
   - Check for conflicts with other PWM devices

4. **Check firmware**
   - Verify `AUDIO_DEFAULT_VOLUME > 0`
   - Ensure `AUDIO_ENABLE_VOICE_ALERTS = true`

### Distorted Audio

1. **Lower volume** - High volume causes clipping
   - `audio_manager.setVolume(50);`

2. **Add power supply filtering**
   - Add 100µF capacitor between VIN and GND

3. **Use 8Ω speaker** instead of 4Ω
   - 4Ω speaker draws higher current, may cause distortion

### Quiet Audio

1. **Check volume setting**
   - `audio_manager.getVolume();` should return ≥60

2. **Check speaker impedance**
   - Verify speaker is 4-8Ω (not 16Ω)

3. **Check power supply**
   - ESP32 power supply may be limiting current under load

## Power Budget During Audio

### Peak Power Scenario

```
Battery: 4000 mAh, 3.7V nominal

Normal operation:     95 mA
Full volume alert:   300 mA
Duration:             5 seconds

Energy used = 300 mA × 5s = 1500 mAs = 0.42 mAh
Battery impact: 4000 mAh ÷ 0.42 mAh = 1 extra minute runtime per alert
```

### Optimization

For extended battery life:
- Use lower volume (50-80%) for non-emergency alerts
- Keep emergency alerts brief (3-5 seconds)
- Disable voice alerts if not needed

## Next Steps

1. **Communication**: See [Communication System](communication.md)
2. **Configuration**: See [Config Reference](../configuration/config-reference.md)
3. **Testing**: See [Component Tests](../testing/component-tests.md)
4. **Troubleshooting**: See [Troubleshooting Guide](../troubleshooting.md)
