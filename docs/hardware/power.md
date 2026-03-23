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

### Normal Monitoring (Baseline — with power optimizations)

Typical power draw during normal fall detection monitoring with all optimizations active:

```
ESP32 (80MHz + modem sleep) ~40 mA
MPU6050 (50Hz)               3.9 mA
BMP280                       0.7 mA
MAX30102 (idle, no collect)  0.6 mA
FSR sensors                  0.6 mA
PAM8302 (idle)              50 mA
──────────────────────────────────
Total Normal (optimized): ~96 mA
```

Without power optimizations (legacy 240MHz, 100Hz, WiFi always-on, MAX30102 collecting):

```
ESP32 (240MHz, WiFi active) 80 mA
MPU6050 (100Hz)              3.9 mA
BMP280                       0.7 mA
MAX30102 (collecting)       10 mA
FSR sensors                  0.6 mA
PAM8302 (idle)              50 mA
──────────────────────────────────
Total Normal (legacy):    ~145 mA
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

## Implemented Power Optimizations

The firmware includes several power-saving features controlled via `Config.h`. These are **active by default** and reduce normal monitoring draw from ~220 mA to ~100-130 mA.

### 1. WiFi Modem Sleep (saves ~60-80 mA)

WiFi radio sleeps between DTIM beacon intervals while staying associated. Enabled automatically after WiFi connects.

```cpp
// Config.h
#define WIFI_POWER_SAVE_MODE WIFI_PS_MIN_MODEM  // or WIFI_PS_MAX_MODEM for aggressive saving
```

`WIFI_PS_MIN_MODEM` sleeps between beacons with minimal latency increase. `WIFI_PS_MAX_MODEM` sleeps more aggressively but may increase response time for incoming packets.

### 2. Reduced Sensor Polling — 50 Hz (saves ~10-20% CPU time)

Fall detection literature works well at 50 Hz. Polling was reduced from 100 Hz with no detection quality loss.

```cpp
// Config.h
#define SENSOR_SAMPLE_RATE_HZ 50
#define SENSOR_READ_INTERVAL_MS 20   // 50Hz
#define MAIN_LOOP_DELAY_MS 20        // 50Hz main loop
```

### 3. On-Demand MAX30102 (saves ~15 mA)

Heart rate sensor collection is disabled during normal monitoring and activated only when a fall is detected. After the alert resolves, collection stops again.

```cpp
// Config.h
#define MAX30102_ALWAYS_ON false  // true = continuous collection (legacy behavior)
```

The sensor remains initialized on the I2C bus — only active LED measurement is toggled via `startCollection()` / `stopCollection()`.

### 4. BLE WiFi Fallback (saves ~10-20 mA when WiFi is available)

BLE is a backup communication channel. When WiFi is connected, BLE is not started. If WiFi drops after max retries, BLE activates as a fallback. When WiFi reconnects, BLE shuts down.

!!! warning "IRAM Constraint"
    Enabling BLE adds ~1.5 KB to IRAM. If your build overflows `iram0_0_seg`, keep this disabled.

```cpp
// Config.h
#define ENABLE_BLE_FALLBACK false  // Set true if IRAM budget allows
```

### 5. Batched WiFi Transmissions (saves ~20-30 mA average)

Sensor data stream interval is increased during normal operation (15s) and reverts to frequent transmission (5s) during active fall alerts.

```cpp
// Config.h
#define SENSOR_STREAM_INTERVAL_MS 15000   // Normal mode
#define SENSOR_STREAM_EMERGENCY_MS 5000   // During active alerts
```

### 6. CPU Frequency Scaling (saves ~20-40 mA)

CPU runs at 80 MHz instead of the default 240 MHz. This is sufficient for sensor polling and WiFi. Increase to 160 MHz if WiFi instability occurs.

```cpp
// Config.h
#define CPU_FREQUENCY_MHZ 80  // 80, 160, or 240
```

### 7. Production Mode — Disable Serial Output (saves ~5-10 mA)

A `PRODUCTION_MODE` flag disables all `Serial` output and debug logging. Set to `true` for deployed devices.

```cpp
// Config.h
#define PRODUCTION_MODE false  // Set true to disable all Serial output
```

When enabled, `DEBUG_SENSOR_DATA`, `DEBUG_ALGORITHM_STEPS`, and `DEBUG_COMMUNICATION` are all forced to `false`.

## Combined Savings Summary

| State | Before | After | Savings |
|-------|--------|-------|---------|
| **Normal monitoring** | ~220 mA | ~100-130 mA | ~40-55% |
| **WiFi TX burst** | ~300 mA | ~250 mA | ~17% |
| **Battery life (2000 mAh)** | ~9 hrs | ~15-20 hrs | ~2x |

## Additional Power Optimization Strategies

### Sleep Mode (not yet implemented)

**Strategy**: Enter deep sleep during inactivity

```
Normal state:          95 mA
With 8-hour sleep:     ~25 mA average
```

Deep sleep can extend battery life by **60-75%** but sacrifices real-time monitoring during sleep.

### Bluetooth Only Mode

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

All power settings are in `SmartFall/Config.h`:

```cpp
// Power Management
#define PRODUCTION_MODE             false
#define WIFI_POWER_SAVE_MODE        WIFI_PS_MIN_MODEM
#define CPU_FREQUENCY_MHZ           80
#define ENABLE_BLE_FALLBACK         false
#define MAX30102_ALWAYS_ON          false
#define SENSOR_STREAM_INTERVAL_MS   15000
#define SENSOR_STREAM_EMERGENCY_MS  5000

// Timing (50Hz polling)
#define SENSOR_SAMPLE_RATE_HZ       50
#define SENSOR_READ_INTERVAL_MS     20
#define MAIN_LOOP_DELAY_MS          20

// Battery
#define BATTERY_LOW_THRESHOLD       3.3f

// Audio
#define AUDIO_DEFAULT_VOLUME        80     // 0-100 (higher = more current)
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
