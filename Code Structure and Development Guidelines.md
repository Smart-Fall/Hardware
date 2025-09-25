# SmartFall Code Structure and Development Guidelines

**Project**: SmartFall Wearable Fall Detection System  
**Document Version**: 1.0  
**Date**: September 24, 2025  
**Target Platform**: ESP32 Feather V2 (Arduino Framework)  
**Language**: C/C++  

---

## 1. Overview

This document defines the modular code structure and development guidelines for the SmartFall project. The architecture follows a **single-file-per-module** approach to maximize team collaboration, code maintainability, and parallel development capabilities.

## 2. Project Architecture Philosophy

### 2.1 Design Principles
- **Single Responsibility**: Each module handles one specific functionality
- **Interface-Based Design**: Clean separation between header (.h) and implementation (.cpp)
- **Minimal Dependencies**: Modules include only necessary dependencies
- **Hardware Abstraction**: Sensor-specific code isolated from algorithm logic
- **Interrupt-Driven Architecture**: Efficient handling of time-critical events
- **Memory Conscious**: Optimized for ESP32's 8MB Flash and 2MB PSRAM

### 2.2 Development Benefits
- **Parallel Development**: 14+ independent modules for simultaneous development
- **Version Control Friendly**: Minimal merge conflicts with separate files
- **Testing Isolation**: Individual module unit testing capability
- **Clear Ownership**: Specific team members responsible for specific modules
- **Incremental Integration**: Modules can be integrated progressively

## 3. Directory Structure

```
SmartFall/
├── src/
│   ├── main.cpp                    // System coordinator and main loop
│   │
│   ├── sensors/                    // Hardware sensor modules
│   │   ├── bmi323_sensor.cpp/.h    // BMI-323 Accelerometer/Gyroscope
│   │   ├── bmp280_sensor.cpp/.h    // BMP-280 Barometric pressure
│   │   ├── max30102_sensor.cpp/.h  // MAX30102 Heart rate sensor
│   │   └── fsr_sensor.cpp/.h       // Analog FSR (optional)
│   │
│   ├── detection/                  // Fall detection algorithm
│   │   ├── fall_detector.cpp/.h    // Multi-stage detection logic
│   │   └── confidence_scorer.cpp/.h // Confidence scoring system
│   │
│   ├── alerts/                     // Alert and notification systems
│   │   ├── audio_alert.cpp/.h      // PAM8302 + Speaker audio alerts
│   │   ├── haptic_alert.cpp/.h     // DRV2605 + Motor haptic feedback
│   │   └── visual_alert.cpp/.h     // TFT display notifications
│   │
│   ├── communication/              // Data transmission modules
│   │   ├── bluetooth_comm.cpp/.h   // Bluetooth Low Energy (BLE)
│   │   ├── wifi_comm.cpp/.h        // WiFi HTTP/HTTPS communication
│   │   └── emergency_comm.cpp/.h   // Emergency protocol coordination
│   │
│   ├── interface/                  // User interaction modules
│   │   └── sos_button.cpp/.h       // SOS button interrupt handling
│   │
│   ├── system/                     // System management modules
│   │   └── power_management.cpp/.h // Battery monitoring and power control
│   │
│   ├── utils/                      // Shared utilities and configuration
│   │   ├── config.h                // System-wide constants and thresholds
│   │   ├── data_types.h            // Common data structures
│   │   └── debug_utils.cpp/.h      // Debug logging and diagnostics
│   │
│   └── tests/                      // Unit testing modules (optional)
│       ├── sensor_tests.cpp/.h     // Sensor module tests
│       ├── algorithm_tests.cpp/.h  // Algorithm validation tests
│       └── integration_tests.cpp/.h // System integration tests
│
├── libraries/                      // External dependencies
│   ├── Adafruit_BMI323/           // BMI-323 sensor library
│   ├── Adafruit_BMP280/           // BMP-280 sensor library
│   ├── MAX30105lib/               // MAX30102 sensor library
│   └── ...                        // Additional required libraries
│
├── docs/                          // Project documentation
│   ├── algorithm_specification.md // Fall detection algorithm document
│   ├── hardware_integration.md   // Hardware setup and wiring
│   ├── api_documentation.md      // Module interface documentation
│   └── testing_procedures.md     // Testing and validation procedures
│
├── config/                        // Configuration files
│   ├── pin_definitions.h         // ESP32 pin mappings
│   ├── sensor_calibration.h      // Calibration constants
│   └── thresholds.h              // Algorithm threshold parameters
│
├── platformio.ini                // PlatformIO project configuration
└── README.md                     // Project overview and setup instructions
```

## 4. Module Specifications

### 4.1 Sensor Modules

#### **bmi323_sensor.cpp/.h**
**Purpose**: Interface with BMI-323 6-axis inertial measurement unit
**Key Functions**:
```cpp
class BMI323Sensor {
public:
    bool init();
    bool readAcceleration(float& x, float& y, float& z);
    bool readAngularVelocity(float& x, float& y, float& z);
    float getTotalAcceleration();
    float getAngularMagnitude();
    void setSampleRate(uint16_t rate_hz);
    void enterSleepMode();
    bool isDataReady();
};
```

#### **bmp280_sensor.cpp/.h**
**Purpose**: Barometric pressure sensing for altitude change detection
**Key Functions**:
```cpp
class BMP280Sensor {
public:
    bool init();
    float readPressure();
    float readAltitude();
    float getAltitudeChange();
    void resetAltitudeBaseline();
    bool isDataReady();
};
```

#### **max30102_sensor.cpp/.h**
**Purpose**: Heart rate and SpO2 monitoring
**Key Functions**:
```cpp
class MAX30102Sensor {
public:
    bool init();
    bool readHeartRate(float& bpm);
    bool isHeartRateValid();
    float getHeartRateBaseline();
    float getHeartRateChange();
    void calibrateBaseline();
};
```

#### **fsr_sensor.cpp/.h**
**Purpose**: Force-sensitive resistor for impact and strap tension detection
**Key Functions**:
```cpp
class FSRSensor {
public:
    bool init(uint8_t analog_pin);
    uint16_t readRawValue();
    float readPressure();
    bool detectImpact();
    bool isStrapSecure();
    void calibrateBaseline();
};
```

### 4.2 Detection Algorithm Modules

#### **fall_detector.cpp/.h**
**Purpose**: Multi-stage fall detection algorithm implementation
**Key Functions**:
```cpp
class FallDetector {
public:
    bool init();
    void processSensorData(SensorData_t& data);
    FallStatus_t getCurrentStatus();
    void resetDetection();
    bool isMonitoring();
    void setThresholds(DetectionThresholds_t& thresholds);
    
private:
    bool checkStage1_FreeFall(SensorData_t& data);
    bool checkStage2_Impact(SensorData_t& data);
    bool checkStage3_Rotation(SensorData_t& data);
    bool checkStage4_Inactivity(SensorData_t& data);
    void applyFalsePositiveFilters(SensorData_t& data);
};
```

#### **confidence_scorer.cpp/.h**
**Purpose**: Confidence scoring system for fall detection stages
**Key Functions**:
```cpp
class ConfidenceScorer {
public:
    void resetScore();
    void addStage1Score(float duration, float magnitude);
    void addStage2Score(float impact, float timing, bool fsr_detected);
    void addStage3Score(float angular_velocity, float orientation_change);
    void addStage4Score(float inactivity_duration, bool stable);
    void addFilterScores(bool pressure, bool heart_rate, bool fsr);
    uint8_t getTotalScore();
    FallConfidence_t getConfidenceLevel();
};
```

### 4.3 Alert System Modules

#### **audio_alert.cpp/.h**
**Purpose**: Audio alert generation using PAM8302 amplifier and speaker
**Key Functions**:
```cpp
class AudioAlert {
public:
    bool init();
    void playEmergencyBeep();
    void playVoiceMessage(VoiceMessage_t message);
    void playCountdown(uint8_t seconds);
    void stopAll();
    bool isPlaying();
    void setVolume(uint8_t volume);
};
```

#### **haptic_alert.cpp/.h**
**Purpose**: Haptic feedback using DRV2605 driver and vibration motor
**Key Functions**:
```cpp
class HapticAlert {
public:
    bool init();
    void playEmergencyPattern();
    void playSinglePulse();
    void playCountdownPulse();
    void stopVibration();
    bool isActive();
    void setIntensity(uint8_t intensity);
};
```

#### **visual_alert.cpp/.h**
**Purpose**: Visual notifications on optional TFT display
**Key Functions**:
```cpp
class VisualAlert {
public:
    bool init();
    void showFallDetected();
    void showCountdown(uint8_t seconds);
    void showStatus(SystemStatus_t status);
    void showBatteryLevel(uint8_t percentage);
    void clear();
    bool isDisplayAvailable();
};
```

### 4.4 Communication Modules

#### **bluetooth_comm.cpp/.h**
**Purpose**: Bluetooth Low Energy communication with mobile devices
**Key Functions**:
```cpp
class BluetoothComm {
public:
    bool init();
    bool isConnected();
    bool sendEmergencyAlert(EmergencyData_t& data);
    bool sendStatusUpdate(StatusData_t& data);
    bool receiveConfiguration(Config_t& config);
    void handleIncomingData();
    void disconnect();
};
```

#### **wifi_comm.cpp/.h**
**Purpose**: WiFi connectivity and HTTP/HTTPS communication
**Key Functions**:
```cpp
class WiFiComm {
public:
    bool init();
    bool connectToNetwork(const char* ssid, const char* password);
    bool isConnected();
    bool sendEmergencyAlert(EmergencyData_t& data);
    bool sendStatusUpdate(StatusData_t& data);
    bool syncConfiguration();
    void disconnect();
};
```

#### **emergency_comm.cpp/.h**
**Purpose**: Emergency communication protocol coordination
**Key Functions**:
```cpp
class EmergencyComm {
public:
    bool init();
    void triggerEmergencyAlert(FallData_t& fall_data);
    void sendSOSAlert(uint32_t timestamp);
    bool sendTestAlert();
    void setEmergencyContacts(ContactList_t& contacts);
    bool isEmergencyActive();
    void cancelEmergency();
};
```

### 4.5 Interface and System Modules

#### **sos_button.cpp/.h**
**Purpose**: SOS button interrupt handling and debouncing
**Key Functions**:
```cpp
class SOSButton {
public:
    bool init(uint8_t pin);
    bool isPressed();
    void enableInterrupt();
    void disableInterrupt();
    uint32_t getLastPressTime();
    void setCallback(void (*callback)());
    
private:
    static void IRAM_ATTR buttonISR();
    void debouncePress();
};
```

#### **power_management.cpp/.h**
**Purpose**: Battery monitoring and power optimization
**Key Functions**:
```cpp
class PowerManagement {
public:
    bool init();
    float getBatteryVoltage();
    uint8_t getBatteryPercentage();
    bool isCharging();
    bool isLowBattery();
    void enterSleepMode(uint32_t duration_ms);
    void enablePowerSaving();
    void disablePowerSaving();
};
```

## 5. Data Structures and Types

### 5.1 Common Data Types (data_types.h)

```cpp
// Sensor data structure
typedef struct {
    float accel_x, accel_y, accel_z;          // Acceleration (g)
    float gyro_x, gyro_y, gyro_z;             // Angular velocity (°/s)
    float pressure;                            // Barometric pressure (hPa)
    float heart_rate;                          // Heart rate (BPM)
    uint16_t fsr_value;                        // FSR reading (ADC counts)
    uint32_t timestamp;                        // Timestamp (ms)
    bool valid;                                // Data validity flag
} SensorData_t;

// Fall detection status
typedef enum {
    FALL_STATUS_MONITORING,
    FALL_STATUS_STAGE1_FREEFALL,
    FALL_STATUS_STAGE2_IMPACT,
    FALL_STATUS_STAGE3_ROTATION,
    FALL_STATUS_STAGE4_INACTIVITY,
    FALL_STATUS_POTENTIAL_FALL,
    FALL_STATUS_FALL_DETECTED,
    FALL_STATUS_EMERGENCY_ACTIVE
} FallStatus_t;

// Confidence levels
typedef enum {
    CONFIDENCE_NO_FALL = 0,
    CONFIDENCE_SUSPICIOUS = 1,
    CONFIDENCE_POTENTIAL = 2,
    CONFIDENCE_CONFIRMED = 3,
    CONFIDENCE_HIGH = 4
} FallConfidence_t;

// Emergency data payload
typedef struct {
    uint32_t timestamp;
    FallConfidence_t confidence;
    uint8_t confidence_score;
    SensorData_t sensor_history[100];  // 10-second history at 10Hz
    float battery_level;
    bool sos_triggered;
    char device_id[32];
} EmergencyData_t;
```

### 5.2 Configuration Structure (config.h)

```cpp
// System configuration constants
#define SENSOR_SAMPLE_RATE_HZ       100
#define DETECTION_WINDOW_MS         10000
#define ALERT_TIMEOUT_MS           30000
#define BATTERY_LOW_THRESHOLD      3.3f

// Algorithm thresholds
#define FREEFALL_THRESHOLD_G       0.5f
#define IMPACT_THRESHOLD_G         3.0f
#define ROTATION_THRESHOLD_DPS     250.0f
#define INACTIVITY_THRESHOLD_MS    2000

// Pin definitions
#define BMI323_CS_PIN              5
#define BMP280_CS_PIN              6
#define MAX30102_INT_PIN           21
#define FSR_ANALOG_PIN             A0
#define SOS_BUTTON_PIN             38
#define SPEAKER_PIN                25
#define HAPTIC_ENABLE_PIN          27
#define BATTERY_SENSE_PIN          A13
```

## 6. Development Guidelines

### 6.1 Coding Standards

**File Naming Conventions**:
- **Header files**: `module_name.h` (lowercase, underscore-separated)
- **Source files**: `module_name.cpp` (matching header file name)
- **Class names**: `ModuleName` (PascalCase)
- **Function names**: `functionName` (camelCase)
- **Constants**: `CONSTANT_NAME` (uppercase, underscore-separated)

**Code Structure**:
```cpp
// Header file template (module_name.h)
#ifndef MODULE_NAME_H
#define MODULE_NAME_H

#include "data_types.h"
#include <Arduino.h>

class ModuleName {
private:
    // Private member variables
    
public:
    // Constructor/Destructor
    ModuleName();
    ~ModuleName();
    
    // Public interface functions
    bool init();
    // ... other functions
};

#endif // MODULE_NAME_H
```

### 6.2 Memory Management

**Stack Usage Guidelines**:
- Avoid large local variables (>1KB)
- Use dynamic allocation sparingly
- Prefer static allocation for sensor buffers
- Monitor stack usage in deep call chains

**PSRAM Utilization**:
- Store large data buffers (sensor history) in PSRAM
- Use `ps_malloc()` for PSRAM allocation
- Keep frequently accessed data in internal RAM

### 6.3 Interrupt Handling

**Interrupt Service Routines (ISR)**:
```cpp
// ISR guidelines
void IRAM_ATTR buttonISR() {
    // Keep ISR code minimal and fast
    // Set flags, don't perform complex operations
    // Use IRAM_ATTR for ISR functions
}
```

### 6.4 Error Handling

**Return Value Conventions**:
- `true/false` for success/failure operations
- `nullptr` for failed pointer returns
- Error codes for detailed error information
- Always validate sensor readings before use

### 6.5 Testing Standards

**Unit Testing Framework**:
- Each module must include test functions
- Test critical paths and edge cases
- Mock hardware dependencies for testing
- Validate input parameter ranges

## 7. Integration Strategy

### 7.1 Development Phases

**Phase 1: Interface Definition** (Week 1)
- Define all header files with function signatures
- Create stub implementations returning dummy values
- Ensure project compiles successfully

**Phase 2: Individual Module Development** (Weeks 2-4)
- Implement sensor modules (parallel development)
- Implement detection algorithm modules
- Implement alert and communication modules

**Phase 3: Integration Testing** (Weeks 5-6)
- Integrate sensor modules with detection algorithm
- Integrate alert systems with detection results
- Integrate communication systems

**Phase 4: System Validation** (Weeks 7-8)
- Complete system testing with real hardware
- Performance optimization and tuning
- Documentation completion

### 7.2 Module Dependencies

**Dependency Hierarchy**:
1. **Level 0**: Hardware abstraction (sensor modules)
2. **Level 1**: Algorithm logic (detection modules)
3. **Level 2**: Response systems (alert modules)
4. **Level 3**: Communication systems (data transmission)
5. **Level 4**: System coordination (main.cpp)

### 7.3 Build System Configuration

**PlatformIO Configuration** (platformio.ini):
```ini
[env:esp32featherv2]
platform = espressif32
board = adafruit_feather_esp32_v2
framework = arduino

lib_deps = 
    adafruit/Adafruit BMI323@^1.0.0
    adafruit/Adafruit BMP280 Library@^2.6.8
    adafruit/MAX30105 library@^1.2.3
    
build_flags = 
    -D CORE_DEBUG_LEVEL=3
    -D CONFIG_ARDUHAL_LOG_COLORS
    
monitor_speed = 115200
upload_speed = 921600
```

## 8. Simulation and Testing Strategy with Wokwi

### 8.1 Simulation-First Development Approach

The SmartFall project employs a **simulation-first development strategy** using Wokwi, an online electronics simulator that supports ESP32 and a wide variety of sensors and components. This approach enables immediate algorithm development and system testing without waiting for physical hardware procurement.

### 8.2 Component Mapping for Simulation

Since Wokwi may not have exact matches for all SmartFall components, we utilize **functionally equivalent components** that provide the same data types, communication protocols, and behavior patterns:

#### **Direct Hardware Equivalents**
| **SmartFall Component** | **Wokwi Equivalent** | **Compatibility Level** |
|------------------------|---------------------|-------------------------|
| **ESP32 Feather V2** | **ESP32 DevKit v1** | ✅ Perfect - Same ESP32 architecture |
| **SOS Button** | **Push Button (wokwi-pushbutton)** | ✅ Perfect - GPIO interrupt capability |
| **Optional TFT Display** | **LCD1602/SSD1306 OLED** | ✅ Perfect - I2C/SPI protocols |

#### **Functional Sensor Equivalents**
| **SmartFall Component** | **Wokwi Equivalent** | **Testing Capability** |
|------------------------|---------------------|------------------------|
| **BMI-323 (6-axis IMU)** | **MPU6050** | ✅ Same 3-axis accelerometer + gyroscope data |
| **BMP-280 (Pressure)** | **DHT22** | ✅ I2C communication, environmental sensor data |
| **MAX30102 (Heart Rate)** | **Potentiometer** | ✅ Analog readings, variable sensor simulation |
| **Analog FSR** | **Potentiometer** | ✅ Perfect analog sensor equivalent |
| **Audio/Haptic Alerts** | **LEDs** | ✅ Visual feedback for alert state testing |

### 8.3 Wokwi Simulation Architecture

#### **Complete Simulation Setup**
```
ESP32 DevKit v1 (Wokwi)
├── MPU6050 (BMI-323 simulation) → I2C (SDA: 21, SCL: 22)
├── DHT22 (BMP-280 simulation) → Digital Pin 4
├── Potentiometer #1 (MAX30102 simulation) → A0 (Heart rate simulation)
├── Potentiometer #2 (FSR simulation) → A1 (Force/impact simulation)
├── Push Button (SOS) → GPIO 38 (with pull-up)
├── LED Array (Alert indicators) → GPIO 25, 26, 27
├── LCD1602 (Optional display) → I2C (SDA: 21, SCL: 22)
└── Serial Monitor → USB (debugging and data output)
```

#### **Wokwi Project Templates**
- **Algorithm Development**: `https://wokwi.com/projects/new/esp32`
- **Component Integration**: ESP32 + MPU6050 + Push Button + LEDs
- **Communication Testing**: ESP32 + WiFi simulation ("Wokwi-GUEST" network)
- **Complete System**: All components integrated for end-to-end testing

### 8.4 Code Abstraction for Hardware Independence

#### **Hardware Abstraction Layer Implementation**
To ensure seamless transition between simulation and real hardware, implement sensor abstraction:

```cpp
// Abstract sensor interface (works for both simulation and real hardware)
class AccelerometerInterface {
public:
    virtual bool init() = 0;
    virtual bool readAcceleration(float& x, float& y, float& z) = 0;
    virtual float getTotalAcceleration() = 0;
};

// Wokwi simulation implementation
class MPU6050_Simulator : public AccelerometerInterface {
private:
    Adafruit_MPU6050 mpu;
public:
    bool init() override {
        return mpu.begin();
    }
    bool readAcceleration(float& x, float& y, float& z) override {
        sensors_event_t event;
        mpu.getAccelerometerSensor()->getEvent(&event);
        x = event.acceleration.x;
        y = event.acceleration.y;
        z = event.acceleration.z;
        return true;
    }
};

// Real hardware implementation (for later migration)
class BMI323_Hardware : public AccelerometerInterface {
private:
    BMI323 bmi323;
public:
    bool init() override {
        return bmi323.initialize();
    }
    bool readAcceleration(float& x, float& y, float& z) override {
        return bmi323.readAcceleration(&x, &y, &z);
    }
};
```

### 8.5 Simulation Testing Phases

#### **Phase 1: Algorithm Logic Testing**
**Duration**: Current sprint (Algorithm development)
**Focus**: Core fall detection algorithm validation
**Components**: ESP32 + MPU6050 + Push Button + LEDs

**Testing Scenarios**:
- **Normal Activity Patterns**: Walking, sitting, standing
- **Fall Simulation**: Controlled sensor data patterns
- **False Positive Testing**: Dropping device, sudden movements
- **SOS Button**: Manual emergency activation
- **Threshold Tuning**: Optimize detection sensitivity

#### **Phase 2: System Integration Testing**
**Duration**: Integration sprint
**Focus**: Multi-module coordination and communication
**Components**: Full simulation setup with all equivalent sensors

**Testing Scenarios**:
- **Multi-sensor Coordination**: Simultaneous sensor readings
- **Alert System Testing**: LED patterns, LCD display output
- **Communication Protocols**: WiFi connection, data transmission
- **Power Management**: Sleep modes, wake-up interrupts
- **State Machine Validation**: Complete system workflow

#### **Phase 3: Hardware Migration Validation**
**Duration**: Hardware testing phase
**Focus**: Verify simulation results match real hardware behavior
**Components**: Actual SmartFall hardware components

**Migration Tasks**:
- **Pin Mapping Updates**: Update pin definitions for Feather V2
- **Library Replacements**: Swap simulation libraries for hardware libraries
- **Calibration**: Adjust thresholds based on real sensor characteristics
- **Performance Validation**: Confirm timing and power consumption

### 8.6 Simulation Benefits for Team Development

#### **Parallel Development Advantages**:
- **Immediate Start**: Begin development without hardware procurement delays
- **Team Collaboration**: Multiple developers can test the same simulation
- **Version Control**: Share simulation configurations via GitHub
- **Debugging Tools**: Logic analyzer, serial monitor, variable inspection
- **Safe Testing**: Test dangerous scenarios (falls) without risk

#### **Algorithm Development Benefits**:
- **Rapid Iteration**: Instantly test algorithm changes
- **Controlled Testing**: Precise sensor data input for edge cases
- **Visual Debugging**: Monitor decision tree progression with LEDs
- **Data Logging**: Capture sensor patterns for analysis
- **Threshold Optimization**: Systematic parameter tuning

### 8.7 Wokwi Integration with Development Workflow

#### **VS Code Integration**
Wokwi provides VS Code extension for seamless development:
- **Direct Simulation**: Run simulations from VS Code
- **Code Upload**: Automatic firmware compilation and upload
- **Debugging Integration**: Breakpoints and variable monitoring
- **Project Templates**: Pre-configured SmartFall simulation setups

#### **Continuous Integration Testing**
- **Automated Testing**: Wokwi CLI for automated simulation runs
- **Regression Testing**: Validate algorithm changes don't break functionality
- **Performance Monitoring**: Track execution timing and resource usage
- **Documentation**: Generate test reports from simulation results

### 8.8 Simulation Limitations and Considerations

#### **Known Limitations**:
- **Sensor Accuracy**: Simulated sensors may not perfectly match real hardware characteristics
- **Timing Precision**: Simulation timing may differ from real-time execution
- **Power Consumption**: Simulation cannot accurately predict battery life
- **Environmental Factors**: Limited simulation of temperature, vibration effects

#### **Mitigation Strategies**:
- **Conservative Thresholds**: Use simulation for algorithm logic, real hardware for final tuning
- **Multiple Test Scenarios**: Test wide range of conditions in simulation
- **Hardware Validation**: Always validate critical paths with real hardware
- **Documentation**: Record simulation assumptions and limitations

## 9. Team Collaboration Guidelines

### 8.1 Module Ownership Assignment

**Recommended module distribution for 6-person team**:
- **Developer 1**: Sensor modules (bmi323, bmp280, max30102, fsr)
- **Developer 2**: Detection algorithm (fall_detector, confidence_scorer)
- **Developer 3**: Alert systems (audio, haptic, visual)
- **Developer 4**: Communication (bluetooth, wifi, emergency)
- **Developer 5**: Interface & system (sos_button, power_management)
- **Developer 6**: Integration & testing (main.cpp, system coordination)

### 8.2 Version Control Strategy

**Git Workflow**:
- **Main branch**: Stable, tested code only
- **Develop branch**: Integration branch for module development
- **Feature branches**: Individual module development (`feature/module-name`)
- **Pull requests**: Required for all merges to develop/main

**Commit Guidelines**:
- Commit message format: `[MODULE] Brief description`
- Example: `[BMI323] Implement accelerometer reading functions`
- Include module name in commit messages for clarity

### 8.3 Documentation Requirements

**Per-Module Documentation**:
- Function documentation in header files
- Implementation notes in source files
- Usage examples for complex modules
- Hardware setup instructions for sensor modules

**Integration Documentation**:
- Module interaction diagrams
- Data flow documentation  
- Error handling procedures
- Testing and validation results