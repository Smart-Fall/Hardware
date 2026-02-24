# Power Budget & Battery Life

Detailed power consumption analysis and battery life calculations for SmartFall.

## Power Consumption by Component

### Microcontroller

| State | Current | Notes |
|-------|---------|-------|
| **Active (WiFi/BLE)** | ~80 mA | Normal operation |
| **Deep Sleep** | ~70 µA | Minimum power state |
| **Standby** | ~10 mA | Sensors off, waiting |

### Sensors

| Sensor | Current | Operating Mode |
|--------|---------|-----------------|
| **MPU6050** | 3.9 mA | Continuous 100Hz sampling |
| **BMP280** | 0.7 mA | Single-shot mode (periodic) |
| **MAX30102** | 10 mA | Continuous heart rate measurement |
| **FSR (combined)** | 0.6 mA | Passive analog inputs |

### Audio System

| Component | Current | Notes |
|-----------|---------|-------|
| **PAM8302 (idle)** | 50 mA | Powered but no audio |
| **PAM8302 (25% volume)** | ~100 mA | Quiet alert |
| **PAM8302 (50% volume)** | ~150 mA | Normal alert |
| **PAM8302 (75% volume)** | ~250 mA | Loud alert |
| **PAM8302 (100% volume)** | ~300 mA | Maximum loudness |

### Optional Outputs

| Component | Current | Notes |
|-----------|---------|-------|
| **Visual LED** | ~20 mA | When on |
| **Haptic Motor** | 50-100 mA | Vibration active |

## System Operating States

### Normal Monitoring (Baseline)

Typical power draw during normal fall detection monitoring:

```
ESP32 (active)          80 mA
MPU6050                  3.9 mA
BMP280                   0.7 mA
MAX30102                10 mA
FSR sensors              0.6 mA
PAM8302 (idle)          50 mA
──────────────────────────────
Total Normal:           95 mA
```

### Peak Power (Emergency Alert)

Maximum power draw during active emergency alert:

```
ESP32 (active)          80 mA
All Sensors            15 mA
PAM8302 (100% volume) 300 mA
Haptic Motor          100 mA
Visual LED             20 mA
──────────────────────────────
Total Peak:           515 mA
```

### Typical Alert Duration

Emergency alert system operation pattern:

- **Audio duration**: 3-5 seconds (high power)
- **Haptic duration**: 5 seconds (medium power)
- **Visual LED**: 30 seconds (low power)
- **Alert transmission**: 2-3 seconds (medium power)

### Battery Life Calculations

#### 4000 mAh Battery (Common)

**Scenario 1: Normal Operation (40 falls/month)**

```
Normal monitoring (95 mA):  ~42 hours
Less alert time (515 mA × 30s × 40/month):  ~0.3 hours
──────────────────────────────────────────
Estimated battery life:   40-42 hours / month
```

**Scenario 2: Continuous Monitoring (24/7)**

```
Normal operation: 4000 mAh ÷ 95 mA = 42 hours
```

#### 5000 mAh Battery (Larger)

**Scenario 1: Normal Operation (40 falls/month)**

```
Estimated battery life:   50-52 hours / month
```

**Scenario 2: Continuous Monitoring (24/7)**

```
Normal operation: 5000 mAh ÷ 95 mA = 52 hours
```

## Power Optimization Strategies

### 1. WiFi Power Saving

**Strategy**: Reduce WiFi connectivity frequency

| Approach | Current | Notes |
|----------|---------|-------|
| Always-on WiFi | 80 mA | Fastest emergency alerts |
| 10s periodic scan | 40 mA | Balanced approach |
| BLE-only (WiFi off) | 10 mA | Lowest power, mobile alerts only |

### 2. Sensor Sampling Rate

**Strategy**: Adjust sensor sampling frequency

| Frequency | Current | Notes |
|-----------|---------|-------|
| 100 Hz (current) | 15 mA | Full sensitivity |
| 50 Hz | 8 mA | Acceptable for fall detection |
| 10 Hz | 2 mA | Reduced responsiveness |

### 3. Sleep Mode

**Strategy**: Enter deep sleep during inactivity

```
Normal state:          95 mA
With 8-hour sleep:     ~25 mA average
```

Deep sleep can extend battery life by **60-75%** but sacrifices real-time monitoring during sleep.

### 4. Bluetooth Only Mode

For low-power scenarios (mobile app monitoring only):

```
Without WiFi:          ~15 mA
With BLE advertising:  ~20 mA
Savings vs WiFi:       75% reduction
```

## Recommended Battery Specifications

| Aspect | Specification | Reason |
|--------|---------------|--------|
| **Chemistry** | LiPo 3.7V nominal | Standard for ESP32 |
| **Capacity** | 4000-5000 mAh | 40-50 hour operation |
| **Discharge Rate** | 1C minimum | Supports peak 500+ mA |
| **Protection** | BMS recommended | Overcharge/short protection |

## Charging Characteristics

### USB-C Charging (Feather V2)

| Parameter | Value |
|-----------|-------|
| **Input Voltage** | 5V USB |
| **Typical Charge Current** | 500 mA |
| **Charge Time** | ~8-10 hours (4000 mAh) |
| **Full Charge Detection** | Auto (integrated circuitry) |

### Charging Efficiency

```
Typical LiPo charging: ~90% efficient
Heat dissipation: ~10% of input power
```

## Runtime Capacity Chart

```mermaid
xychart-beta
    title Battery Capacity vs Operating Time (at 95mA average)
    x-axis [40, 50, 60, 70, 80, 90] hours
    y-axis "Battery Capacity" 1000 --> 5000 mAh
    line [1000, 2000, 3000, 4000, 5000, 5000]
    line [1000, 2000, 3000, 4000, 4500, 4000]
    line [1000, 2000, 3000, 3500, 3000, 2000]
```

**Operating Time at 95mA (Normal Operation)**:
- **1000 mAh**: ~10 hours
- **2000 mAh**: ~21 hours
- **3000 mAh**: ~32 hours
- **4000 mAh**: ~42 hours
- **5000 mAh**: ~53 hours

## Power Management Configuration

Edit `SmartFall/Config.h`:

```cpp
// Battery Configuration
#define BATTERY_LOW_THRESHOLD      3.3f   // Voltage for low battery warning
#define ENABLE_DEEP_SLEEP          false  // Experimental: deep sleep mode
#define WIFI_POWER_SAVE_ENABLED    false  // Experimental: WiFi power saving

// Sensor Sampling
#define SENSOR_SAMPLE_RATE_HZ      100    // 100Hz for full sensitivity
                                          // Reduce to 50Hz for 50% power savings

// Audio System
#define AUDIO_DEFAULT_VOLUME       80     // 0-100 (higher = more current)
#define AUDIO_IDLE_TIMEOUT_MS      5000   // Auto-mute after 5s
```

## Battery Monitoring

### Reading Battery Voltage

The system monitors battery voltage via GPIO 36 (A4) using a voltage divider.

**Formula:**
```
Actual Voltage = (ADC_reading / 4095) × 3.3V × (100k + 50k) / 50k
               = ADC_reading × 0.00161
```

### Battery Level Warnings

| Voltage | State | Action |
|---------|-------|--------|
| **> 3.6V** | Healthy | Normal operation |
| **3.3-3.6V** | Low | Audio/haptic warning |
| **< 3.3V** | Critical | Disable WiFi/audio to extend runtime |

## Next Steps

1. **Component Details**: See [Hardware Overview](overview.md)
2. **Wiring Diagram**: See [Wiring Guide](wiring.md)
3. **Sensor Specs**: See [Firmware Sensors](../firmware/sensors.md)
4. **Testing**: See [Component Tests](../testing/component-tests.md)

## References

- ESP32 Datasheet: [Espressif Resources](https://www.espressif.com/en/products/socs/esp32)
- PAM8302 Datasheet: [Power Audio Amplifier](https://www.diodes.com/assets/Uploads/PAM8302A.pdf)
- LiPo Battery Safety: [Adafruit LiPo Guide](https://learn.adafruit.com/adafruit-feather-m0-adalogger/power-management)
