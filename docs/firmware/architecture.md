# Firmware Architecture

Comprehensive overview of the SmartFall firmware structure, modules, and dependencies.

## Project Directory Structure

```
SmartFall/
├── SmartFall.ino                 # Main sketch (entry point)
├── sketch.yaml                   # Arduino CLI configuration
│
├── Config.h                      # All system configuration
├── Data_Types.h                  # Shared data structures
│
├── Sensors/
│   ├── MPU6050.h/cpp            # 6-axis IMU driver
│   ├── BMP280.h/cpp             # Pressure sensor driver
│   ├── MAX30102_Sensor.h/cpp     # Heart rate sensor driver
│   └── FSR.h/cpp                # Force sensor driver
│
├── Detection/
│   ├── Fall_Detector.h/cpp       # 5-stage fall detection algorithm
│   └── Confidence_Scorer.h/cpp   # Confidence scoring system
│
├── Communication/
│   ├── WiFi_Manager.h/cpp        # WiFi connectivity & HTTP
│   ├── BLE_Server.h/cpp          # Bluetooth Low Energy server
│   ├── Emergency_Comms.h/cpp     # Alert transmission logic
│   └── Log_Manager.h/cpp         # Remote logging system
│
├── Audio/
│   └── Audio_Manager.h/cpp       # PAM8302 audio output system
│
└── Tests/                        # Component test sketches
    ├── MPU6050/
    ├── BMP280/
    ├── MAX30102/
    ├── FSR/
    ├── WiFi/
    ├── BLE/
    └── Audio/
```

## Module Dependency Diagram

```mermaid
graph TD
    A["SmartFall.ino<br/>(Main Loop)"] --> B["Config.h<br/>(Configuration)"]
    A --> C["Data_Types.h<br/>(Structures)"]

    A --> D["Sensors"]
    D --> D1["MPU6050"]
    D --> D2["BMP280"]
    D --> D3["MAX30102"]
    D --> D4["FSR"]

    A --> E["Fall Detection"]
    E --> E1["Fall_Detector"]
    E --> E2["Confidence_Scorer"]
    E1 --> C
    E2 --> C

    A --> F["Communication"]
    F --> F1["WiFi_Manager"]
    F --> F2["BLE_Server"]
    F --> F3["Emergency_Comms"]
    F --> F4["Log_Manager"]
    F1 --> C
    F2 --> C
    F3 --> C
    F4 --> F1

    A --> G["Audio"]
    G --> G1["Audio_Manager"]
    G1 --> B

    D1 --> B
    D2 --> B
    D3 --> B
    D4 --> B

    E1 --> D
    E2 --> D
    F3 --> E1
    F3 --> E2
```

## Main Loop Flow

The `SmartFall.ino` main sketch follows this execution pattern:

```mermaid
graph TD
    A["System Startup"]
    A1["✓ Initialize sensors<br/>✓ Configure GPIO pins<br/>✓ Start WiFi/BLE<br/>✓ Print startup banner"]
    A --> A1

    A1 --> B["Main Loop<br/>10ms / 100Hz"]

    B --> B1["(1) Read all<br/>sensor data"]
    B1 --> B2["(2) Pass to<br/>Fall_Detector"]
    B2 --> B3["(3) Check SOS<br/>button"]
    B3 --> B4["(4) Process<br/>emergency alert"]
    B4 --> B5["(5) Update WiFi/<br/>BLE status"]
    B5 --> B6["(6) Transmit alert<br/>if needed"]
    B6 --> B7["(7) Update UI<br/>LED/audio"]
    B7 --> B8["(8) Sleep for<br/>remaining time"]

    B8 --> C["Loop every 10ms"]
    C -->|Continue| B

    style A fill:#4ade80,color:#000
    style B fill:#60a5fa,color:#fff
    style C fill:#f97316,color:#fff
```

## Core Data Structures

### SensorData_t

```cpp
struct SensorData_t {
    // Timestamp
    uint32_t timestamp_ms;

    // Accelerometer (m/s²)
    float accel_x, accel_y, accel_z;

    // Gyroscope (°/s)
    float gyro_x, gyro_y, gyro_z;

    // Pressure sensor
    float pressure_pa;
    float altitude_m;
    float temperature_c;

    // Heart rate
    uint16_t heart_rate;        // BPM
    uint8_t spo2;               // Blood oxygen %
    float heart_rate_temperature; // °C

    // Force sensor
    uint16_t fsr_value;         // 0-4095 ADC
    float device_pressure;      // Estimated Pa

    // Battery
    float battery_voltage;      // Volts
    uint8_t battery_percent;    // 0-100%
};
```

### FallStatus_t

```cpp
enum FallStatus_t {
    NO_FALL_DETECTED = 0,
    SUSPICIOUS_ACTIVITY = 1,
    POTENTIAL_FALL = 2,
    CONFIRMED_FALL = 3,
    HIGH_CONFIDENCE_FALL = 4,
    SOS_TRIGGERED = 5
};
```

## Module Responsibilities

### Configuration (`Config.h`)

Central repository for all configurable parameters:
- Pin definitions (I2C, analog, digital)
- Threshold values (acceleration, rotation, confidence)
- WiFi/BLE credentials
- Timing constants
- Audio settings
- Debug flags

### Sensors Module

**Individual drivers:**

| Driver | Responsibility |
|--------|-----------------|
| **MPU6050** | Read 6-axis IMU data @ 100Hz |
| **BMP280** | Read pressure & temperature |
| **MAX30102** | Read heart rate & SpO2 |
| **FSR** | Read analog force sensor |

**Interface:**
```cpp
bool init();              // Initialize sensor
bool readData(sensor_data);  // Get latest reading
bool isAvailable();       // Check if responding
```

### Fall Detection Pipeline

**Flow:**
```mermaid
graph TD
    A["Raw Sensor Data<br/>(Accel, Gyro, Pressure, HR)"]

    B["Fall_Detector<br/>• 5-stage analysis<br/>• Timing validation<br/>• History tracking"]

    C["Confidence_Scorer<br/>• Calculate points<br/>• Apply thresholds<br/>• Return FallStatus_t"]

    D["FallStatus_t Result<br/>(0-5 enum value)"]

    A --> B
    B --> C
    C --> D

    style A fill:#3b82f6,color:#fff
    style B fill:#8b5cf6,color:#fff
    style C fill:#d946ef,color:#fff
    style D fill:#dc2626,color:#fff
```

### Communication System

**Architecture:**
```mermaid
graph TD
    A["Emergency Alert<br/>Triggered"]

    B["WiFi_Manager<br/>sendEmergency()"]
    B1["HTTP POST<br/>/api/emergency"]

    C["BLE_Server<br/>sendEmergencyAlert()"]
    C1["Notify Emergency<br/>Characteristic"]

    D["Alert Transmitted<br/>WiFi + BLE"]

    A --> B
    A --> C
    B --> B1
    C --> C1
    B1 --> D
    C1 --> D

    style A fill:#dc2626,color:#fff
    style B fill:#f97316,color:#fff
    style C fill:#f97316,color:#fff
    style D fill:#16a34a,color:#fff
```

**Features:**
- Automatic retry on failure
- Dual-protocol redundancy
- Offline queueing (experimental)

### Audio System

**Capabilities:**
- Pre-defined alert patterns
- Voice-like tone sequences
- Volume control (0-100%)
- PWM synthesis at 5 kHz

## Key Design Principles

### 1. Modularity

Each component (sensors, detection, communication) is independent with clear interfaces.

**Benefit**: Easy testing, replacement, and parallel development

### 2. Memory Efficiency

- Circular buffers for sensor history
- No dynamic allocation in core loops
- Fixed-size arrays matching ESP32 constraints

**Benefit**: Predictable memory usage, no fragmentation

### 3. Real-Time Processing

- 100Hz sampling rate
- Non-blocking main loop
- Hardware interrupts for SOS button

**Benefit**: Sub-2-second response time to falls

### 4. Configurability

All thresholds in `Config.h` without recompilation.

**Benefit**: Quick tuning for different user profiles

## Build Configuration

### Arduino CLI

```bash
cd SmartFall
arduino-cli compile --fqbn esp32:esp32:adafruit_feather_esp32_v2 .
```

### PlatformIO

```bash
pio run -e feather_esp32_v2 -t build
```

### Arduino IDE

File → Open → SmartFall.ino, then Sketch → Verify

## Memory Usage

### Flash Memory

```
Firmware binary:        ~600 KB
Partition table:        ~10 KB
Reserved SPIFFS:        ~1.4 MB
─────────────────────
Total 8MB Flash
```

### RAM Usage

```
Static data:            ~80 KB
Sensor history buffer:  ~10 KB (100 samples × 100 bytes)
Stack:                  ~20 KB
Free heap:              ~400+ KB (available for runtime)
```

## Performance Characteristics

| Metric | Value | Notes |
|--------|-------|-------|
| **Sensor Sample Rate** | 100 Hz | 10ms intervals |
| **Fall Detection Time** | <2 seconds | From event to alert |
| **Alert Transmission** | <1 second | WiFi POST request |
| **CPU Utilization** | <5% | Dual-core 240MHz |
| **Loop Jitter** | ±1ms | Acceptable for fall detection |

## Next Steps

1. **Sensor Details**: See [Sensors](sensors.md)
2. **Detection Logic**: See [Fall Detection](fall-detection.md)
3. **Communication**: See [Communication](communication.md)
4. **Audio**: See [Audio System](audio.md)
5. **Configuration**: See [Config Reference](../configuration/config-reference.md)
