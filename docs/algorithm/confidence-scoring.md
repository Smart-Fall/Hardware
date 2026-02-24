# Confidence Scoring & Classification

Final stage of the SmartFall algorithm: converting raw stage points into actionable alerts.

## Total Confidence Calculation

The confidence score is the sum of all stage points:

```
Total Confidence = Stage1_Points + Stage2_Points + Stage3_Points
                   + Stage4_Points + Stage5_Points

Maximum Possible = 25 + 25 + 20 + 20 + 15 = 105 points
```

### Example Calculation

**Scenario: Elderly person falls backward from standing**

```
Stage 1 (Free Fall):        18 pts  (350ms duration, 0.2g magnitude)
Stage 2 (Impact):           20 pts  (5.5g impact, immediate timing)
Stage 3 (Rotation):         14 pts  (380°/s rotation, 75° change)
Stage 4 (Inactivity):       18 pts  (3.2s motionless)
Stage 5 (Filters):          12 pts  (0.8m altitude drop + HR spike)
────────────────────────────────────
TOTAL CONFIDENCE:           82 pts

Classification: HIGH_CONFIDENCE_FALL → Immediate Alert
```

## Classification Thresholds

SmartFall uses these fixed thresholds to classify falls:

!!! warning "Threshold Values"
    **Config.h values are authoritative.** These differ from the specification:
    - Config.h: 76/67/48/29
    - Specification: 80/70/50/30

    Use the code values below.

### Threshold Definitions

```
Confidence Scale (0-105 points)

     105 ┤
         │
      80 ├─────────┐
         │         │ HIGH_CONFIDENCE_FALL (≥76)
      76 ├────┐    │ Immediate alert, no delay
         │    │    │
      70 ├────┤────┴─ CONFIRMED_FALL (67-75)
         │    │       5-second delay before alert
      67 ├────┘
         │
      60 ├──────────┐
         │          │ POTENTIAL_FALL (48-66)
      50 ├──┐       │ Extended monitoring (10s)
         │  │       │
      48 ├──┘───────┘
         │
      40 ├──────────┐
         │          │ SUSPICIOUS_ACTIVITY (30-47)
      30 ├──────────┘ Normal monitoring, increased sensitivity
         │
       0 └──────────── NO_FALL_DETECTED (<30)
```

### Detailed Threshold Table

| Score | Status | Action | Delay | Confidence |
|-------|--------|--------|-------|-----------|
| **≥ 76** | HIGH_CONFIDENCE_FALL | Immediate alert | 0 sec | 95%+ |
| **67-75** | CONFIRMED_FALL | Alert after delay | 5 sec | 85% |
| **48-66** | POTENTIAL_FALL | Enhanced monitor | — | 60% |
| **30-47** | SUSPICIOUS_ACTIVITY | Normal monitor | — | 30% |
| **< 30** | NO_FALL_DETECTED | Reset | — | <10% |

### Actual Config.h Values

```cpp
#define HIGH_CONFIDENCE_THRESHOLD  76    // ≥76: immediate alert
#define CONFIRMED_THRESHOLD        67    // 67-75: 5s delay alert
#define POTENTIAL_THRESHOLD        48    // 48-66: enhanced monitor
#define SUSPICIOUS_THRESHOLD       29    // 30-47: normal monitor
                                         // <30: no fall
```

## Fall Status Enum

```cpp
enum FallStatus_t {
    NO_FALL_DETECTED = 0,           // Score < 30
    SUSPICIOUS_ACTIVITY = 1,        // Score 30-47
    POTENTIAL_FALL = 2,             // Score 48-66
    CONFIRMED_FALL = 3,             // Score 67-75
    HIGH_CONFIDENCE_FALL = 4,       // Score ≥76
    SOS_TRIGGERED = 5               // Manual button press
};
```

## Alert Decision Matrix

```mermaid
graph TD
    A["Confidence Score Calculated"] --> B{Score Range?}

    B -->|≥ 76| C["[HIGH CONFIDENCE]<br/>FALL"]
    B -->|67-75| D["[CONFIRMED]<br/>FALL"]
    B -->|48-66| E["[POTENTIAL]<br/>FALL"]
    B -->|30-47| F["[SUSPICIOUS]<br/>ACTIVITY"]
    B -->|&lt; 30| G["[CLEAR]<br/>NO FALL DETECTED"]

    C --> C1["✓ Immediate alert<br/>✓ Play siren<br/>✓ Notify emergency<br/>✓ 30s countdown"]
    C1 --> C2{User Response?}
    C2 -->|Moves| C3["Cancel alert"]
    C2 -->|SOS button| C4["Confirm emergency"]
    C2 -->|No response| C5["Contact services"]

    D --> D1["→ 5-second delay<br/>Monitor score"]
    D1 --> D2{Score remains high?}
    D2 -->|≥67| D3["Trigger alert"]
    D2 -->|&lt;67 or movement| D4["Cancel, resume monitoring"]

    E --> E1["📊 Enhanced monitoring<br/>+10 second window"]
    E1 --> E2{Escalation<br/>or recovery?}
    E2 -->|Escalate| E3["Score ≥67<br/>→ Confirmed Fall"]
    E2 -->|Recover| E4["-30 points<br/>→ No Fall"]

    F --> F1["Continue monitoring<br/>Increased sensitivity"]

    G --> G1["Reset detection<br/>Return to baseline"]

    style C fill:#ff4444,color:#fff
    style D fill:#ff9944,color:#000
    style E fill:#ffdd44,color:#000
    style F fill:#4444ff,color:#fff
    style G fill:#cccccc,color:#000
```

### Detailed Thresholds

**HIGH_CONFIDENCE_FALL (≥76 points)**
- **Action**: IMMEDIATE alert (no delay)
- **Rationale**: 76+ points indicates a highly confident fall. Delay would endanger the user.
- **Response Options**:
  - User moves significantly → Cancel alert
  - SOS button pressed → Confirm emergency
  - No response (30s) → Contact emergency services

**CONFIRMED_FALL (67-75 points)**
- **Action**: DELAYED alert (5-second wait)
- **Rationale**: At this borderline score, 5 seconds allows the person to move if they're OK, preventing false alarms.
- **Monitoring**: Check if score remains ≥67 or if user moves

**POTENTIAL_FALL (48-66 points)**
- **Action**: Enhanced monitoring mode (10 more seconds)
- **Rationale**: Score suggests possible fall, but recovery is likely. Extended monitoring with escalation logic.
- **Can escalate to**: CONFIRMED_FALL if inactivity continues
- **Can downgrade to**: NO_FALL if clear recovery detected

**SUSPICIOUS_ACTIVITY (30-47 points)**
- **Action**: Continue normal monitoring with increased sensitivity
- **Rationale**: Some criteria met, but score too low for alert
- **Possible causes**: Exercise, quick recovery, false trigger
- **Next step**: Recalculate if additional motion detected

**NO_FALL_DETECTED (<30 points)**
- **Action**: Reset detection state, return to baseline monitoring
- **Rationale**: Sensors haven't detected fall pattern

## Enhanced Monitoring Decision Logic

When confidence is POTENTIAL_FALL (48-66), SmartFall enters enhanced monitoring:

### Recovery Signals (Downgrade)

If any of these occur, **subtract 30 points**:

```mermaid
graph LR
    A["Recovery Signal<br/>Detected"] --> B{"Signal Type?"}

    B -->|Coordinated Movement| C["Roll → Sit → Stand<br/>within 5 seconds"]
    B -->|Achieves Upright| D["Acceleration pattern<br/>changes to upright"]
    B -->|Walking Pattern| E["Regular stride<br/>100-120 steps/min"]
    B -->|Audio Response| F["Movement within<br/>10 seconds of prompt"]

    C --> G["−30 Points<br/>Score Drops"]
    D --> G
    E --> G
    F --> G

    G --> H{New Score < 30?}
    H -->|Yes| I["[OK] NO_FALL_DETECTED<br/>Resume baseline"]
    H -->|No| J["Continue monitoring"]

    style A fill:#4ade80,color:#000
    style I fill:#cccccc,color:#000
```

**Result**: Confidence drops below 30 → NO_FALL_DETECTED, resume normal monitoring

### Escalation Signals (Upgrade)

If any of these occur, **add points**:

```mermaid
graph LR
    A["Escalation Signal<br/>Detected"] --> B{"Signal Type?"}

    B -->|Continued Inactivity| C["+8 points<br/>No movement 5+ sec<br/>after impact"]
    B -->|Elevated Heart Rate| D["+5 points<br/>HR remains 30+ BPM<br/>above baseline"]
    B -->|Device Orientation| E["+3 points<br/>Sustained 0.9-1.1g<br/>on non-gravity axis"]
    B -->|No Audio Response| F["+5 points<br/>No movement after<br/>audio prompt"]

    C --> G["Update Score"]
    D --> G
    E --> G
    F --> G

    G --> H{Score ≥ 67?}
    H -->|Yes| I["[ALERT] CONFIRMED_FALL<br/>Trigger 5s delay alert"]
    H -->|No| J["Continue enhanced<br/>monitoring"]

    style A fill:#ef4444,color:#fff
    style I fill:#ff9944,color:#000
```

**Result**: Score reaches ≥67 → CONFIRMED_FALL, trigger alert with 5-second delay

## SOS Button Override

The SOS button (GPIO 15) bypasses all detection stages:

```cpp
if (sosButtonPressed) {
    confidence_score = 100;  // Override
    status = SOS_TRIGGERED;
    playEmergencyAlert();
    sendEmergencyNotification();
}
```

**SOS Logic**:
- Any time: Immediate emergency alert
- During fall detection: Confirms that the event is real
- Purpose: User manual override for medical emergencies

## Scoring Example: False Positive Scenario

**Scenario**: Dropping device from desk while standing

```
Stage 1: Brief free fall (0.3s) = 10 pts
Stage 2: No impact (device caught) = 0 pts
         ────────────────────────────
Subtotal: 10 pts

OR

Stage 1: Free fall detected = 10 pts
Stage 2: High acceleration (caught by hand) = 8 pts
         (But no sustained inactivity)
Stage 3: Minor rotation = 0 pts
Stage 4: Immediate recovery (standing still) = 0 pts
         ────────────────────────────────
Subtotal: 18 pts

Status: NO_FALL_DETECTED (< 30) ✓ Correctly avoided false alarm
```

## Scoring Example: True Fall Scenario

**Scenario**: Actual fall down two steps

```
Stage 1: Free fall 400ms = 10 pts (0.2g minimum)
Stage 2: Impact 4.2g = 12 pts + timing 3 pts + FSR 7 pts = 22 pts
Stage 3: Rotation 350°/s = 8 pts + 70° change = 3 pts = 11 pts
Stage 4: Inactivity 3.5s = 8 pts + stability 5 pts = 13 pts
Stage 5: Altitude 1.2m = 3 pts + HR spike = 5 pts = 8 pts
         ────────────────────────────────────────────────
TOTAL: 10 + 22 + 11 + 13 + 8 = 64 pts

Status: POTENTIAL_FALL (48-66)
Action: Enhanced monitoring for 10 seconds
        If person doesn't get up → Escalate to CONFIRMED_FALL
```

## Practical Implications

### Emergency First Responders

```
Score < 30:    May call back to verify
Score 30-66:   Optional verification call before dispatch
Score ≥67:     Dispatch emergency services immediately
```

### Mobile App Display

```
Score Visualization:
  0-29:    Green "OK" indicator
 30-47:    Yellow "Caution" indicator
 48-66:    Orange "Alert" with monitoring status
 67-75:    Red "Fall Detected" with 5-second countdown
 ≥76:      Red "Emergency" with immediate dispatch

Real-time updates sent to app every 100ms
```

## Configuration Adjustments

To modify sensitivity, edit `Config.h`:

```cpp
// More sensitive (catches more falls, more false positives)
#define HIGH_CONFIDENCE_THRESHOLD  70    // Lower threshold
#define CONFIRMED_THRESHOLD        60
#define POTENTIAL_THRESHOLD        40

// Less sensitive (misses more falls, fewer false positives)
#define HIGH_CONFIDENCE_THRESHOLD  85    // Higher threshold
#define CONFIRMED_THRESHOLD        75
#define POTENTIAL_THRESHOLD        55
```

## Next Steps

1. **Testing**: See [Fall Simulation Tests](../testing/fall-simulation.md)
2. **Implementation**: See [Fall Detection Code](../firmware/fall-detection.md)
3. **Troubleshooting**: See [Troubleshooting Guide](../troubleshooting.md)
4. **Configuration**: See [Config Reference](../configuration/config-reference.md)
