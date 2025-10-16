/*
 * SmartFall - Audio System Test (PAM8302)
 *
 * Tests PAM8302 2.5W Class D Audio Amplifier with various tones,
 * alert patterns, and voice-like sequences
 *
 * Hardware: ESP32 HUZZAH32 Feather + PAM8302 Amplifier
 *
 * Wiring:
 * - PAM8302 A+ → ESP32 GPIO 25 (PWM output)
 * - PAM8302 A- → GND
 * - PAM8302 Vin → 3.3V or USB (5V)
 * - PAM8302 GND → GND
 * - Speaker (4-8Ω) → PAM8302 + and - terminals
 *
 * This test verifies:
 * - PWM tone generation
 * - Volume control (0-100%)
 * - Single/multiple tone playback
 * - Alert patterns (beeps, siren, urgent, SOS)
 * - Voice-like alert sequences
 * - Startup melody
 * - Fall detection audio sequence
 */

#include "Audio_Manager.h"

#define SPEAKER_PIN  25  // ESP32 GPIO25 (PWM capable)

Audio_Manager audioManager(SPEAKER_PIN);

void setup() {
    Serial.begin(115200);
    delay(2000);

    Serial.println("\n========================================");
    Serial.println("    SmartFall Audio System Test");
    Serial.println("      PAM8302 Amplifier Test");
    Serial.println("========================================\n");

    // Test 1: Audio System Initialization
    Serial.println("TEST 1: Audio System Initialization");
    Serial.println("------------------------------------");
    if (audioManager.begin()) {
        Serial.println("✓ Audio manager initialized");
        Serial.println("✓ PWM configured on GPIO 25");
        audioManager.setVolume(80);  // Set to 80% volume
        Serial.println();
    } else {
        Serial.println("✗ Audio initialization failed!");
        while (true) delay(1000);
    }

    delay(1000);

    // Test 2: Startup Melody
    Serial.println("TEST 2: Startup Melody");
    Serial.println("-----------------------");
    Serial.println("Playing startup melody (5 ascending notes)...");
    audioManager.playStartupMelody();
    Serial.println("✓ Startup melody complete\n");

    delay(2000);

    // Test 3: Volume Control
    Serial.println("TEST 3: Volume Control");
    Serial.println("----------------------");
    Serial.println("Playing tone at different volume levels:");

    uint8_t volumes[] = {25, 50, 75, 100};
    for (int i = 0; i < 4; i++) {
        audioManager.setVolume(volumes[i]);
        Serial.print("  Volume ");
        Serial.print(volumes[i]);
        Serial.println("%");
        audioManager.playTone(1000, 500);
        delay(500);
    }
    audioManager.setVolume(80);  // Reset to 80%
    Serial.println("✓ Volume control test complete\n");

    delay(2000);

    // Test 4: Basic Tones
    Serial.println("TEST 4: Basic Tone Generation");
    Serial.println("------------------------------");
    Serial.println("Playing frequency sweep (500Hz to 2000Hz):");

    for (uint16_t freq = 500; freq <= 2000; freq += 250) {
        Serial.print("  ");
        Serial.print(freq);
        Serial.println(" Hz");
        audioManager.playTone(freq, 300);
        delay(200);
    }
    Serial.println("✓ Tone generation test complete\n");

    delay(2000);

    // Test 5: Alert Patterns
    Serial.println("TEST 5: Alert Patterns");
    Serial.println("-----------------------");

    Serial.println("1. Single Beep:");
    audioManager.playPattern(ALERT_PATTERN_SINGLE_BEEP);
    delay(1000);

    Serial.println("2. Double Beep:");
    audioManager.playPattern(ALERT_PATTERN_DOUBLE_BEEP);
    delay(1000);

    Serial.println("3. Triple Beep:");
    audioManager.playPattern(ALERT_PATTERN_TRIPLE_BEEP);
    delay(1000);

    Serial.println("4. Confirmation Tone:");
    audioManager.playConfirmationTone();
    delay(1000);

    Serial.println("5. Warning Tone:");
    audioManager.playWarningTone();
    delay(1000);

    Serial.println("6. Error Tone:");
    audioManager.playErrorTone();
    delay(1000);

    Serial.println("7. Urgent Pattern:");
    audioManager.playPattern(ALERT_PATTERN_URGENT);
    delay(1000);

    Serial.println("8. Siren Pattern:");
    audioManager.playPattern(ALERT_PATTERN_SIREN);
    delay(1000);

    Serial.println("✓ Alert patterns test complete\n");

    delay(2000);

    // Test 6: SOS Morse Code
    Serial.println("TEST 6: SOS Morse Code");
    Serial.println("----------------------");
    Serial.println("Playing SOS sequence (... --- ...):");
    audioManager.playSOSSequence();
    Serial.println("✓ SOS sequence complete\n");

    delay(2000);

    // Test 7: Fall Detection Sequence
    Serial.println("TEST 7: Fall Detection Audio");
    Serial.println("-----------------------------");
    Serial.println("Playing fall detected alert sequence:");
    audioManager.playFallDetectedSequence();
    Serial.println("✓ Fall detection sequence complete\n");

    delay(2000);

    // Test 8: Voice-Like Alerts
    Serial.println("TEST 8: Voice-Like Alert Sequences");
    Serial.println("-----------------------------------");
    Serial.println("These patterns use varying frequencies and");
    Serial.println("durations to mimic speech prosody:\n");

    Serial.println("1. 'Fall Detected':");
    audioManager.playVoiceAlert(VOICE_ALERT_FALL_DETECTED);
    delay(1500);

    Serial.println("2. 'Press Button':");
    audioManager.playVoiceAlert(VOICE_ALERT_PRESS_BUTTON);
    delay(1500);

    Serial.println("3. 'Calling Help':");
    audioManager.playVoiceAlert(VOICE_ALERT_CALLING_HELP);
    delay(1500);

    Serial.println("4. 'Help Sent':");
    audioManager.playVoiceAlert(VOICE_ALERT_HELP_SENT);
    delay(1500);

    Serial.println("5. 'System Ready':");
    audioManager.playVoiceAlert(VOICE_ALERT_SYSTEM_READY);
    delay(1500);

    Serial.println("6. 'Low Battery':");
    audioManager.playVoiceAlert(VOICE_ALERT_LOW_BATTERY);
    delay(1500);

    Serial.println("7. 'Connection Lost':");
    audioManager.playVoiceAlert(VOICE_ALERT_CONNECTION_LOST);
    delay(1500);

    Serial.println("✓ Voice-like alerts test complete\n");

    delay(2000);

    // All tests complete
    Serial.println("========================================");
    Serial.println("    All Audio Tests Complete!");
    Serial.println("========================================\n");
    Serial.println("Test Summary:");
    Serial.println("✓ Audio system initialization");
    Serial.println("✓ Volume control (25% - 100%)");
    Serial.println("✓ Basic tone generation (500Hz - 2000Hz)");
    Serial.println("✓ Alert patterns (8 types)");
    Serial.println("✓ SOS Morse code");
    Serial.println("✓ Fall detection sequence");
    Serial.println("✓ Voice-like alerts (7 types)");
    Serial.println("\nIf you heard all sounds clearly, the PAM8302");
    Serial.println("audio system is working correctly!\n");

    // Play success melody
    audioManager.playConfirmationTone();
    delay(200);
    audioManager.playConfirmationTone();
}

void loop() {
    // Interactive testing mode
    static unsigned long lastDemo = 0;
    unsigned long currentTime = millis();

    // Play a demo sequence every 30 seconds
    if (currentTime - lastDemo >= 30000) {
        lastDemo = currentTime;

        Serial.println("\n--- Demo Sequence (every 30s) ---");
        Serial.println("Playing random alert...");

        // Play a random alert
        int randomAlert = random(0, 7);
        switch (randomAlert) {
            case 0:
                Serial.println("Alert: Fall Detected");
                audioManager.playFallDetectedSequence();
                break;
            case 1:
                Serial.println("Alert: SOS");
                audioManager.playSOSSequence();
                break;
            case 2:
                Serial.println("Alert: Siren");
                audioManager.playPattern(ALERT_PATTERN_SIREN);
                break;
            case 3:
                Serial.println("Alert: Urgent");
                audioManager.playPattern(ALERT_PATTERN_URGENT);
                break;
            case 4:
                Serial.println("Voice: Fall Detected");
                audioManager.playVoiceAlert(VOICE_ALERT_FALL_DETECTED);
                break;
            case 5:
                Serial.println("Voice: Calling Help");
                audioManager.playVoiceAlert(VOICE_ALERT_CALLING_HELP);
                break;
            case 6:
                Serial.println("Voice: System Ready");
                audioManager.playVoiceAlert(VOICE_ALERT_SYSTEM_READY);
                break;
        }
        Serial.println("Demo complete. Next demo in 30 seconds.\n");
    }

    delay(100);
}

/*
 * Troubleshooting:
 *
 * No sound at all:
 * - Check PAM8302 wiring (A+ to GPIO25, GND to GND, Vin to 3.3V/5V)
 * - Verify speaker is connected to PAM8302 output terminals
 * - Check speaker impedance (should be 4Ω or 8Ω)
 * - Ensure volume is not at 0%
 *
 * Distorted sound:
 * - Lower the volume (try 50-70%)
 * - Check for loose connections
 * - Verify power supply is adequate (PAM8302 can draw up to 1A)
 *
 * Weak sound:
 * - Increase volume
 * - Check speaker impedance (lower is louder, but don't go below 4Ω)
 * - Ensure PAM8302 is powered adequately (use USB for testing)
 *
 * Expected Results:
 * - Clear, audible tones at all frequencies
 * - Smooth volume transitions
 * - Distinct alert patterns
 * - Voice-like alerts should sound like speech prosody
 */
