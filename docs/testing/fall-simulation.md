# Fall Simulation & Testing

Methods for testing the complete fall detection system without actual falls.

## Test Methods Overview

| Method | Safety | Realism | Difficulty |
|--------|--------|---------|-----------|
| **SOS Button** | ✓ Safest | ✗ No detection | ✓ Easy |
| **Acceleration Simulation** | ✓ Safe | ◐ Moderate | ◐ Medium |
| **Controlled Drop** | ◐ Careful | ✓ Very realistic | ✗ Difficult |
| **Serial Command** | ✓ Safest | ✗ None | ✓ Easy |

## Method 1: SOS Button Test

**Simplest and safest method**

### Test Procedure

1. Upload SmartFall.ino firmware
2. Press SOS button (GPIO 15)
3. Observe:
   - Audio alert starts immediately
   - LED blinks
   - Serial output: "SOS TRIGGERED"
   - WiFi/BLE alerts sent

### Verification Checklist

- [ ] Audio plays immediately (no delay)
- [ ] Volume appropriate
- [ ] LED indicator activates
- [ ] Serial shows timestamp and alert sent
- [ ] Alerts appear in mobile app (BLE)
- [ ] Server receives notification (WiFi)

### Expected Serial Output

```
[T: 12345ms] SOS button pressed!
[T: 12345ms] ✓ EMERGENCY ALERT TRIGGERED
[T: 12345ms] Fall status: SOS_TRIGGERED (confidence: manual)
[T: 12345ms] Playing emergency siren...
[T: 12350ms] Sending WiFi alert...
[T: 12500ms] ✓ WiFi alert sent successfully
[T: 12510ms] Sending BLE alert...
[T: 12520ms] ✓ BLE alert sent
[T: 12520ms] Alert countdown: 30 seconds...
[T: 13520ms] 29 seconds remaining...
```

### Cancelation

To cancel alert without movement:
- Press SOS button again, OR
- Trigger significant movement

## Method 2: Rapid Movement Simulation

**Mimics fall acceleration patterns**

### Test Procedure

1. **Mount device securely** (avoid dropping)
2. **Rapid downward motion**: Quickly move device downward 1-2 meters
   - Simulates free fall acceleration
   - Triggers Stage 1 detection
3. **Hard horizontal impact**: Tap device against table/floor
   - Simulates impact acceleration
   - Triggers Stage 2 detection
4. **Keep still**: Hold device still on floor
   - Simulates inactivity
   - Triggers Stage 4 detection

### Detailed Steps

```
T=0ms:    Start with device in hand
T=100ms:  Rapidly lower device (acceleration < 0.5g)
T=300ms:  Continue lowering (simulate free fall duration)
T=400ms:  Stop motion and slap device onto table (acceleration > 3g)
T=500ms:  Keep device still on surface
T=2500ms: Continue stillness (2+ seconds)
Result:   Confidence score accumulates → Alert triggers

Expected: Confidence 65-75 pts → CONFIRMED_FALL → 5s delay alert
```

### Verification Checklist

- [ ] Free fall phase detected (Stage 1)
- [ ] Impact detected (Stage 2)
- [ ] Inactivity detected (Stage 4)
- [ ] Confidence score 50+ points
- [ ] Serial debug shows stage progression

### Serial Output Example

```
[00124ms] Stage 1: Free fall detected (duration: 300ms, accel: 0.2g) → +15pts
[00425ms] Stage 2: Impact detected (accel: 4.2g, timing: 125ms) → +18pts
[00500ms] Stage 3: Rotation detected (gyro: 280°/s) → +12pts
[02500ms] Stage 4: Inactivity confirmed (duration: 2s) → +16pts
[02600ms] Stage 5: Altitude change (0.5m drop) → +3pts
[02610ms] ────────────────────────────────────
[02610ms] TOTAL CONFIDENCE: 64 pts → POTENTIAL_FALL
[02610ms] Enhanced monitoring enabled for 10 more seconds...
[02620ms] If no recovery → Escalate to alert
```

## Method 3: Controlled Drop Test

**Most realistic but requires care**

### Safety Precautions

!!! danger "Important"
    - Use low heights (0.5-1.0 meter)
    - Drop onto soft surface (bed, couch, mat)
    - Protect device with padding
    - Never drop from high heights
    - Protect hands and body

### Test Setup

```
Soft landing surface (bed/couch)
           ↓
1 meter ←───
           │
           ↓ Device drops
           │
      ┌─────────────┐
      │   Padding   │
      │   (device)  │
      │   Padding   │
      └─────────────┘
           ↓
      Soft landing
```

### Test Procedure

1. **Secure device** in protective padding/case
2. **Hold at 1 meter height** above soft surface
3. **Release device** - let it fall naturally
4. **Catch before second fall** to prevent damage
5. **Check results** - serial output and alerts

### Expected Behavior

```
T=0ms:     Free fall begins (0.1g accelerometer reading)
T=400ms:   Impact with bed (3-5g acceleration)
T=500ms:   Device comes to rest on surface
T=2500ms:  Still position maintained
T=3000ms:  Confidence score complete

Result: Confidence 70-85 pts → Likely CONFIRMED or HIGH_CONFIDENCE_FALL
Alert:  Immediate siren activation or 5-second delay alert
```

### Verification Checklist

- [ ] All 5 stages triggered in sequence
- [ ] Confidence score ≥ 67 points
- [ ] Audio alert activates
- [ ] Device shows no physical damage
- [ ] Continues normal operation after test

## Method 4: Serial Command Injection (Advanced)

**For developers: inject detection events via serial**

### Test Sketch Code

Create a custom test sketch:

```cpp
#include "Fall_Detector.h"
#include "Confidence_Scorer.h"

FallDetector detector;
ConfidenceScorer scorer;

void setup() {
    Serial.begin(115200);
    detector.init();
    scorer.init();
}

void loop() {
    if (Serial.available()) {
        char cmd = Serial.read();

        if (cmd == '1') {
            // Inject Stage 1 points
            Serial.println("Injecting Stage 1 score (15 points)");
            scorer.addFreeFallScore(15);
        }
        else if (cmd == '2') {
            scorer.addImpactScore(20);
        }
        else if (cmd == '3') {
            scorer.addRotationScore(15);
        }
        else if (cmd == '4') {
            scorer.addInactivityScore(18);
        }
        else if (cmd == '5') {
            scorer.addFilterScore(12);
        }
        else if (cmd == 's') {
            uint8_t total = scorer.getConfidenceScore();
            Serial.print("Current score: ");
            Serial.println(total);
        }
        else if (cmd == 'r') {
            scorer.reset();
            Serial.println("Score reset");
        }
    }
}
```

### Usage

```bash
# Upload test sketch
cd SmartFall/tests/
arduino-cli upload -p PORT --fqbn $BOARD_FQBN .
arduino-cli monitor -p PORT -c baudrate=115200
```

In serial monitor, send commands:
```
1 → Add Stage 1 (15pts) → Total: 15
2 → Add Stage 2 (20pts) → Total: 35
3 → Add Stage 3 (15pts) → Total: 50 (POTENTIAL_FALL threshold!)
4 → Add Stage 4 (18pts) → Total: 68 (CONFIRMED_FALL!)
5 → Add filters (12pts) → Total: 80 (HIGH_CONFIDENCE!)
s → Print current score
r → Reset to 0
```

## Full System Test Checklist

After running fall simulations, verify:

### Detection Pipeline
- [ ] Stage 1 (Free Fall) triggers correctly
- [ ] Stage 2 (Impact) confirms free fall
- [ ] Stage 3 (Rotation) detects spinning
- [ ] Stage 4 (Inactivity) confirms incapacity
- [ ] Stage 5 (Filters) validates with secondary sensors

### Confidence Scoring
- [ ] Points accumulate correctly
- [ ] Thresholds trigger at correct scores:
  - [ ] ≥76 = HIGH_CONFIDENCE
  - [ ] 67-75 = CONFIRMED
  - [ ] 48-66 = POTENTIAL
  - [ ] 30-47 = SUSPICIOUS
  - [ ] <30 = NO_FALL

### Alerts
- [ ] Audio alert triggers:
  - [ ] Immediate for HIGH_CONFIDENCE (≥76)
  - [ ] After 5s for CONFIRMED (67-75)
  - [ ] No audio for POTENTIAL (<67)
- [ ] Volume appropriate (80% default)
- [ ] Multiple patterns play

### Communication
- [ ] WiFi alert sent to server
- [ ] Server responds with 200 OK
- [ ] BLE alert sent to mobile app
- [ ] Both succeed at least partially

### User Interface
- [ ] LED indicator blinks
- [ ] Serial debug output clear
- [ ] 30-second countdown starts
- [ ] Can cancel with movement or button

## Verification Checklist

- [ ] **Sensor Tests**: All component tests pass
- [ ] **Fall Detection**: Multiple simulated falls trigger correctly
- [ ] **Confidence Scoring**: Scores match expectations
- [ ] **Alert System**: Audio, WiFi, BLE all work
- [ ] **User Response Window**: 30-second countdown functional
- [ ] **Alert Cancellation**: Movement detection works
- [ ] **SOS Override**: Button provides immediate alert
- [ ] **Serial Logging**: Debug output informative
- [ ] **Battery Monitoring**: Voltage readings accurate
- [ ] **System Stability**: No crashes, clean shutdown

## Production Readiness Checklist

Before field deployment:

- [ ] All 7 component tests pass completely
- [ ] Fall detection tested with ≥3 different simulation methods
- [ ] WiFi server configured and responding
- [ ] Mobile app BLE connection verified
- [ ] 30-minute continuous monitoring test passed
- [ ] Battery at 100%, lasts >20 hours in normal use
- [ ] Audio clarity acceptable in quiet and noisy environments
- [ ] No spurious alerts during normal daily activity
- [ ] User can understand alert sequence
- [ ] Emergency contacts configured and notified
- [ ] Device initialization under 10 seconds

## Next Steps

1. **Troubleshooting**: If tests fail, see [Troubleshooting Guide](../troubleshooting.md)
2. **Configuration Tuning**: See [Config Reference](../configuration/config-reference.md)
3. **Main System**: Deploy [SmartFall.ino](../getting-started/quick-start.md)
4. **Field Testing**: Begin real-world usage with careful monitoring
