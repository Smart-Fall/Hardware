# Detection Stages

Detailed specification of the 5-stage fall detection pipeline with scoring tables and trigger conditions.

## Stage 1: Pre-Fall Detection (Free Fall Phase)

**Purpose**: Detect the weightlessness phase during a fall

**Trigger Condition**:
- Total acceleration magnitude < 0.5g for duration ≥ 200ms

### Free Fall Physics

When a person falls freely:
- Gravity pulls down at 9.8 m/s² (1g)
- In free fall, there's no supporting force
- Net acceleration ≈ 0g (weightless)
- Actual measurement: <0.5g accounts for body rotation and variations

### Scoring Breakdown

#### Duration Scoring (Time spent falling)

| Duration | Points | Interpretation |
|----------|--------|-----------------|
| < 200ms | 5 pts | Brief drop (possibly stumble) |
| 200-500ms | 10 pts | Typical fall duration |
| > 500ms | 15 pts | Extended fall (high structure) |

**Timing Formula**:
```
If freefall_duration >= 500ms:  award 15 points
Else if freefall_duration >= 200ms: award 10 points
Else if freefall_duration > 0ms:     award 5 points
Else:                               trigger not met
```

#### Magnitude Scoring (How close to zero)

| Acceleration | Points | Interpretation |
|--------------|--------|-----------------|
| 0.3-0.5g | 5 pts | Partial weightlessness (tumbling) |
| 0.1-0.3g | 8 pts | Significant weightlessness |
| < 0.1g | 10 pts | True free fall condition |

**Physics Reference**:
```
Stationary:     1.0g (gravity only)
Free fall:      0.0g (ideal)
Slight rotation: 0.1-0.3g (realistic)
```

### Example: Backward Fall from Chair

```
T=0:    Chair tipping backward
T=50ms: Person loses contact with chair (0.8g)
T=75ms: Body accelerating downward (0.4g)
T=100ms: Mid-fall, approaching free fall (0.15g) ← Magnitude 0.1-0.3g
T=200ms: Continued weightlessness (0.1g)
T=350ms: Still falling (0.2g)

Trigger Analysis:
├─ Duration: 350ms - 0ms = 350ms → Falls in 200-500ms bracket → 10 pts
└─ Magnitude: 0.1-0.3g range → 8 pts

Stage 1 Score: 10 + 8 = 18 pts (out of max 25)
Status: Stage 1 TRIGGERED → Proceed to Stage 2
```

## Stage 2: Impact Detection

**Purpose**: Confirm ground impact following free fall phase

**Trigger Condition**:
- Peak acceleration > 3.0g occurring within 1 second of Stage 1 trigger

### Impact Physics

When free fall ends with ground impact:
- Sudden deceleration from ~7 m/s to ~0 m/s
- Occurs over ~100-300ms contact time
- Peak acceleration: 3-10g depending on surface and body part

### Scoring Breakdown

#### Impact Magnitude (Force of collision)

| Acceleration | Points | Surface Type |
|--------------|--------|--------------|
| 3.0-4.0g | 8 pts | Carpet, foam, grass |
| 4.0-6.0g | 12 pts | Wood, tile, concrete |
| > 6.0g | 15 pts | Hard surface, high velocity |

**Impact Reference Table**:
```
Speed before impact vs. impact acceleration
Fall height  Velocity    Peak Accel    Points
─────────────────────────────────────────
1 meter     4.4 m/s      3-4g           8
1.5 meters  5.4 m/s      4-5g          12
2+ meters   6.3+ m/s     6+g           15
```

#### Timing Accuracy (Was impact immediate?)

| Timing | Points | Interpretation |
|--------|--------|-----------------|
| < 0.5s after free fall | 5 pts | Direct ground contact |
| 0.5-1.0s after free fall | 3 pts | Delayed impact |
| > 1.0s after free fall | 0 pts | Not part of same event |

#### FSR Validation (Device impact confirmation)

| Condition | Points | Notes |
|-----------|--------|-------|
| FSR pressure spike during impact | 7 pts | Device struck ground |
| No FSR spike | 0 pts | Device may have missed impact |

### Example: Fall Hitting Hard Floor

```
T=150ms: Stage 1 triggered (free fall detected)
T=400ms: Head/body hits floor (8g acceleration spike)
T=401ms: FSR shows pressure spike (5000 units)

Trigger Analysis:
├─ Acceleration: 8g → Exceeds 3.0g threshold → Stage 2 TRIGGERED
├─ Impact magnitude: 8g falls in >6.0g bracket → 15 pts
├─ Timing: 400-150 = 250ms = <0.5s → 5 pts
└─ FSR validation: Pressure spike detected → 7 pts

Stage 2 Score: 15 + 5 + 7 = 27 pts (capped at 25 max)
```

## Stage 3: Rotation Assessment

**Purpose**: Validate abnormal body rotation indicative of uncontrolled fall

**Trigger Condition**:
- Angular velocity magnitude > 150°/s during Stages 1-2 timeframe

### Rotation Physics

In a controlled fall (catching yourself):
- Minimal rotational motion
- Body stays upright

In an uncontrolled fall:
- Body rotates as it falls
- Angular velocities: 200-600°/s typical
- Multiple axes of rotation

!!! warning "Threshold Discrepancy"
    Config.h specifies `ROTATION_THRESHOLD_DPS = 150.0f`
    The specification document shows 250°/s
    **Use Config.h value (150°/s) as authoritative**

### Scoring Breakdown

#### Rotational Velocity (Spin speed)

| Angular Velocity | Points | Scenario |
|------------------|--------|----------|
| 150-400°/s | 8 pts | Moderate rotation |
| 400-600°/s | 12 pts | Significant rotation |
| > 600°/s | 15 pts | Severe rotation |

**Reference**:
```
A 180° rotation in:
- 500ms → 360°/s (high spin rate)
- 1000ms → 180°/s (moderate rotation)
```

#### Final Orientation Change (How far did body rotate?)

| Orientation Shift | Points | Recovery |
|-------------------|--------|----------|
| 45-90° change | 3 pts | Partial rotation |
| > 90° change | 5 pts | Major inversion |

### Example: Spinning Fall on Stairs

```
T=100ms: Lost balance on stairs
T=200ms: Free fall triggered (Stage 1)
T=350ms: Massive rotation (520°/s gyro magnitude)
T=400ms: Impact on landing (Stage 2)
T=450ms: Body orientation changed 120°

Trigger Analysis:
├─ Angular velocity: 520°/s → 400-600°/s bracket → 12 pts
└─ Orientation change: 120° → >90° bracket → 5 pts

Stage 3 Score: 12 + 5 = 17 pts (out of max 20)
Status: High rotation confirmed → Likely uncontrolled fall
```

## Stage 4: Post-Impact Inactivity Assessment

**Purpose**: Confirm user inability to recover immediately after impact

**Trigger Condition**:
- Acceleration within 0.8g-1.2g for duration ≥ 2 seconds
- Angular velocity < 50°/s during same period

### Recovery Physics

After a fall, people typically:
- Try to get up (high motion) → False positive avoidance
- Lie still from injury → True fall indicator

Stable acceleration range (0.8-1.2g) indicates:
- Static position on ground
- No significant movement
- Device orientation stable

### Scoring Breakdown

#### Inactivity Duration (How long motionless?)

| Duration | Points | Severity |
|----------|--------|----------|
| 2-5 seconds | 8 pts | Brief incapacitation |
| 5-10 seconds | 12 pts | Moderate incapacitation |
| > 10 seconds | 15 pts | Extended incapacitation |

**Interpretation**:
- 2-5 sec: Person might be dazed, gathering strength
- 5-10 sec: Significant injury, unable to rise quickly
- 10+ sec: Severe injury, loss of consciousness possible

#### Movement Stability (Is person completely still?)

| Motion Pattern | Points | Analysis |
|----------------|--------|----------|
| Minimal micro-movements | 5 pts | Complete stillness |
| No stability | 0 pts | Person moving/recovering |

### Example: Lying on Floor After Fall

```
T=450ms: Impact detected (Stage 2)
T=500ms: Person lands on floor
T=1000ms: Still motionless (0.95g, 2°/s rotation)
T=2000ms: Still motionless (0.98g, 1°/s rotation)
T=3000ms: Still motionless (0.96g, 0°/s rotation)

Trigger Analysis:
├─ Inactivity duration: 3000-500 = 2500ms = 2.5 sec → 8 pts
└─ Movement stability: Micro-movements <5°/s → 5 pts

Stage 4 Score: 8 + 5 = 13 pts (out of max 20)
Status: Person unable to stand → Confirmed incapacity
```

## Stage 5: False Positive Filter System

**Purpose**: Apply secondary sensor validation to reduce false alarms

**Applied after Stages 1-4**, maximum 15 points total

### Filter A: Barometric Pressure Validation (BMP280)

**Logic**: Altitude change distinguishes actual falls from device drops or transportation

#### Altitude Change Scoring

| Altitude Change | Points | Scenario |
|-----------------|--------|----------|
| 0.5-1.0m drop | 2 pts | Single-story fall |
| 1.0-2.0m drop | 3 pts | Two-story fall |
| > 2.0m drop | 5 pts | High elevation fall |

**Physics Reference**:
```
From standing on stairs:  0.5m drop → 1-2 pts
From bed/chair:           0.5m drop → 1-2 pts
From second story window: 4m drop → 5 pts (max)
```

### Filter B: Physiological Response Validation (MAX30102)

**Logic**: Heart rate and oxygen saturation correlate with actual emergency

#### Heart Rate Response Scoring

| HR Change from Baseline | Points | Analysis |
|-------------------------|--------|----------|
| > 40 BPM increase | 8 pts | Strong panic/stress |
| 20-40 BPM increase | 5 pts | Moderate response |
| 10-20 BPM increase | 2 pts | Mild response |
| No change | 0 pts | No physiological stress |

#### SpO2 (Blood Oxygen) Scoring

| Oxygen Level | Points | Status |
|--------------|--------|--------|
| ≥ 90% | 5 pts | Healthy saturation |
| 85-90% | 2 pts | Slightly reduced |
| < 85% | -3 pts | Concerning (may inhibit alert) |

**Interpretation**:
- High HR + good SpO2 = Panic from fall (positive)
- Low HR + good SpO2 = Device drop (negative)
- Low HR + low SpO2 = Medical emergency (positive)

### Filter C: Device Attachment Validation (FSR)

**Logic**: Consistent strap pressure indicates device remains on wearer

#### FSR Validation Scoring

| Condition | Points | Notes |
|-----------|--------|-------|
| Consistent strap tension throughout | 2 pts | Device stayed on |
| Pressure spike during impact phase | 3 pts | Impact confirmed |
| Lost contact (FSR drops to zero) | -5 pts | Device removed/dropped |

## Scoring Summary Table

| Stage | Max Points | Primary Sensor | Validation |
|-------|-----------|-----------------|-----------|
| **1: Free Fall** | 25 | MPU6050 Accel | Duration + magnitude |
| **2: Impact** | 25 | MPU6050 Accel | Timing + FSR |
| **3: Rotation** | 20 | MPU6050 Gyro | Magnitude + angle |
| **4: Inactivity** | 20 | MPU6050 (all axes) | Duration + stability |
| **5: Filters** | 15 | BMP280, MAX30102, FSR | Multi-sensor validation |
| **TOTAL** | **105** | Multi-sensor fusion | Confidence threshold |

## Next Steps

1. **Confidence Thresholds**: See [Confidence Scoring](confidence-scoring.md)
2. **Algorithm Implementation**: See [Fall Detection API](../firmware/fall-detection.md)
3. **Testing**: See [Fall Simulation](../testing/fall-simulation.md)
4. **Configuration**: See [Config Reference](../configuration/config-reference.md)
