# SmartFall Detection Algorithm Specification

**Project**: SmartFall Wearable Fall Detection System  
**Document Version**: 1.0  
**Date**: September 23, 2025  
**Author**: Mohammed Elhasnaoui  
**JIRA Task**: SF-30 - Implement Fall Detection Algorithm Logic  

---

## 1. Executive Summary

The SmartFall detection algorithm employs a multi-stage decision tree approach that analyzes data from multiple sensors to accurately detect fall events while minimizing false positives. The algorithm processes inputs from accelerometer, gyroscope, pressure, heart rate, and optional force sensors through a confidence-based scoring system that culminates in emergency alert activation.

## 2. System Overview

### 2.1 Hardware Components
- **BMI-323**: 3-axis accelerometer and gyroscope (primary fall detection)
- **BMP-280**: Barometric pressure sensor (altitude change detection)
- **MAX30102**: Heart rate sensor (physiological validation)
- **Analog FSR**: Force sensitive resistor (optional impact/strap detection)
- **SOS Button**: Manual emergency override (continuous monitoring)
- **ESP32 Feather V2**: Main controller (240MHz dual-core, 8MB Flash, 2MB PSRAM)

### 2.2 Design Philosophy
The algorithm uses a **staged decision tree** rather than a single mathematical equation to provide:
- **Interpretable logic flow** for debugging and validation
- **Memory-efficient implementation** suitable for microcontroller constraints
- **Tunable threshold parameters** for optimization with real-world data
- **Multiple validation stages** to reduce false positives

## 3. Algorithm Architecture

### 3.1 Processing Flow
The algorithm operates in a continuous monitoring loop with the following stages:

1. **Sensor Data Acquisition** (10ms intervals, 100Hz sampling)
2. **SOS Button Check** (continuous interrupt monitoring)
3. **Multi-Stage Fall Detection** (5-stage sequential analysis)
4. **Confidence Scoring** (weighted point accumulation system)
5. **Decision Logic** (threshold-based classification)
6. **Alert Sequence** (multi-modal user notification)
7. **Emergency Communication** (Bluetooth/WiFi alert transmission)

### 3.2 Timing Requirements
- **Detection Window**: 10-second sliding analysis window
- **Processing Frequency**: 100Hz sensor data processing
- **Alert Response**: <2 seconds from detection to first alert
- **User Response Window**: 30-second countdown for emergency confirmation

## 4. Multi-Stage Fall Detection Logic

### 4.1 Stage 1: Pre-Fall Detection (Free Fall Phase)
**Purpose**: Detect the weightlessness phase during a fall

**Trigger Condition**:
- Total acceleration magnitude < 0.5g for duration ≥ 200ms

**Scoring System** (Maximum 25 points):
- **Duration Scoring**:
  - < 200ms: +5 points (brief drop)
  - 200-500ms: +10 points (typical fall duration)
  - \> 500ms: +15 points (extended fall)
- **Magnitude Scoring**:
  - 0.3-0.5g: +5 points (partial weightlessness)
  - 0.1-0.3g: +8 points (significant weightlessness)
  - < 0.1g: +10 points (true free fall condition)

**Exit Conditions**:
- Continue to Stage 2 if triggered
- Reset monitoring if not triggered within timeout

### 4.2 Stage 2: Impact Detection
**Purpose**: Confirm ground impact following free fall phase

**Trigger Condition**:
- Peak acceleration > 3.0g occurring within 1 second of Stage 1 trigger

**Scoring System** (Maximum 25 points):
- **Impact Magnitude**:
  - 3.0-4.0g: +8 points (moderate impact)
  - 4.0-6.0g: +12 points (significant impact)
  - \> 6.0g: +15 points (severe impact)
- **Timing Accuracy**:
  - < 0.5s after free fall: +5 points (immediate impact)
  - 0.5-1.0s after free fall: +3 points (delayed impact)
- **FSR Validation** (if available):
  - Pressure spike detected: +7 points (device impact confirmation)

**Exit Conditions**:
- Continue to Stage 3 if triggered
- Reset monitoring if impact not detected within window

### 4.3 Stage 3: Orientation Analysis
**Purpose**: Validate abnormal body rotation indicative of uncontrolled fall

**Trigger Condition**:
- Angular velocity magnitude > 250°/s during Stages 1-2 timeframe

**Scoring System** (Maximum 20 points):
- **Rotational Velocity**:
  - 250-400°/s: +8 points (moderate rotation)
  - 400-600°/s: +12 points (significant rotation)
  - \> 600°/s: +15 points (severe rotation)
- **Final Orientation Change**:
  - 45-90° change: +3 points (moderate orientation shift)
  - \> 90° change: +5 points (major orientation shift)

**Exit Conditions**:
- Continue to Stage 4 if triggered
- Reset monitoring if rotation criteria not met

### 4.4 Stage 4: Post-Impact Inactivity Assessment
**Purpose**: Confirm user inability to recover immediately after impact

**Trigger Condition**:
- Acceleration remains within 0.8g < magnitude < 1.2g for duration ≥ 2 seconds
- Angular velocity < 50°/s during same period

**Scoring System** (Maximum 20 points):
- **Inactivity Duration**:
  - 2-5 seconds: +8 points (brief incapacitation)
  - 5-10 seconds: +12 points (moderate incapacitation)
  - \> 10 seconds: +15 points (extended incapacitation)
- **Movement Stability**:
  - Minimal micro-movements detected: +5 points (complete stillness)

**Exit Conditions**:
- Continue to Stage 5 (False Positive Filters) if triggered
- Reset monitoring if significant movement detected

### 4.5 Stage 5: False Positive Filter System
**Purpose**: Apply secondary sensor validation to reduce false alarms

#### Filter A: Barometric Pressure Validation (BMP-280)
**Logic**: Detect altitude change during fall sequence
- **Scoring**:
  - 0.5-1.0m altitude change: +2 points
  - 1.0-2.0m altitude change: +3 points
  - \> 2.0m altitude change: +5 points
- **Purpose**: Distinguish actual falls from device drops or transportation events

#### Filter B: Physiological Response Validation (MAX30102)
**Logic**: Analyze heart rate response to stress event
- **Scoring**:
  - Normal HR maintained (60-100 BPM): +2 points
  - HR increase 10-30 BPM: +3 points
  - HR spike > 30 BPM above baseline: +5 points
- **Purpose**: Correlate physiological stress response with fall event

#### Filter C: Device Attachment Validation (Optional FSR)
**Logic**: Confirm device remains attached to user throughout sequence
- **Scoring**:
  - Consistent strap tension maintained: +2 points
  - Tension spike during impact phase: +3 points
- **Purpose**: Eliminate false positives from device removal or dropping

## 5. Decision Logic and Classification

### 5.1 Confidence Score Calculation
**Total Possible Score**: 105 points
- Stage 1 (Free Fall): 25 points maximum
- Stage 2 (Impact): 25 points maximum
- Stage 3 (Rotation): 20 points maximum
- Stage 4 (Inactivity): 20 points maximum
- Stage 5 (Filters): 15 points maximum

### 5.2 Classification Thresholds
- **≥ 80 points**: HIGH CONFIDENCE FALL
  - Action: Immediate alert sequence activation
- **70-79 points**: CONFIRMED FALL
  - Action: Alert sequence with 5-second delay
- **50-69 points**: POTENTIAL FALL
  - Action: Enhanced monitoring mode (10-second extension)
- **30-49 points**: SUSPICIOUS ACTIVITY
  - Action: Continue standard monitoring with increased sensitivity
- **< 30 points**: NO FALL DETECTED
  - Action: Reset system and resume normal monitoring

## 6. Enhanced Monitoring Mode (Potential Fall State)

### 6.1 Monitoring Duration
**Primary Window**: 10 seconds of enhanced observation
**Extension Window**: Additional 10 seconds if partial recovery detected

### 6.2 Recovery Indicators Analysis
**Movement Pattern Recognition**:
- **Coordinated Recovery Sequences**: Roll → Sit → Stand progression
- **Self-Help Attempts**: Arm/hand movements indicating assistance efforts
- **Stability Achievement**: Minimal swaying in upright position
- **Locomotion Resumption**: Normal walking pattern detection

**Physiological Monitoring**:
- **Heart Rate Trending**: Recovery toward baseline vs. sustained elevation
- **Stress Response Patterns**: Panic indicators vs. normal recovery

**Environmental Context**:
- **Position Stability**: Consistent device orientation
- **Location Consistency**: Stable barometric pressure readings

### 6.3 Enhanced Monitoring Decision Matrix
**Upgrade to FALL DETECTED** (+20 points) if:
- No coordinated recovery movements AND
- Heart rate remains elevated > 20 BPM above baseline AND
- Device orientation suggests person remains down AND
- No user response to gentle audio prompts

**Downgrade to NO FALL** (-30 points) if:
- Clear recovery movement sequence detected OR
- Stable upright position achieved OR
- Normal walking pattern resumed OR
- User responds to status inquiry

## 7. SOS Button Integration

### 7.1 Continuous Monitoring
The SOS button operates as a **highest priority interrupt** throughout all system states:
- **Hardware**: GPIO with internal pull-up resistor (active low)
- **Response Time**: < 100ms interrupt handling
- **Power Consumption**: Minimal current draw in idle state

### 7.2 SOS Activation Scenarios
1. **During Normal Monitoring**: Immediate manual emergency override
2. **During Fall Detection**: Confirmation of emergency situation
3. **During Enhanced Monitoring**: User distress indication
4. **During Alert Sequence**: Emergency confirmation bypass

### 7.3 SOS Response Logic
- **Single Press**: Initiate immediate emergency alert sequence
- **Action**: Bypass all detection stages and scoring
- **Communication**: Direct emergency notification to all configured contacts

## 8. Alert and Communication System

### 8.1 Multi-Modal Alert Sequence
**Audio Alerts** (PAM8302 + Mini Speaker):
- Immediate loud beep pattern (3 beeps, 1-second intervals)
- Voice announcement: "Fall detected! Press button if okay"
- Escalating volume if no response detected

**Haptic Alerts** (Optional DRV2605 + Vibration Motor):
- Strong vibration pattern (5-second duration)
- Repeat every 10 seconds until user response
- Silent alert option for discrete notification

**Visual Alerts** (Optional TFT Display):
- "FALL DETECTED" message with countdown timer
- Battery level and connection status indication
- User instruction prompts

### 8.2 User Response Window
**Duration**: 30-second countdown from first alert
**Response Options**:
- **SOS Button Press**: Immediate emergency confirmation
- **Significant Movement**: Automatic alert cancellation
- **No Response**: Proceed to emergency communication

### 8.3 Emergency Communication Protocol
**Bluetooth Path**:
- Immediate alert transmission to paired mobile application
- Local alert processing and emergency contact notification

**WiFi Path**:
- HTTP/HTTPS request to web server infrastructure
- Real-time alert dashboard notification
- Backup communication redundancy

**Data Payload**:
- Fall confidence score and contributing factors
- Complete sensor reading history (10-second window)
- User identification and timestamp
- Device location data (if available)
- Battery status and device health information

---

**Document Status**: Algorithm Logic Design Complete  
**Next Steps**: Implementation and testing phase