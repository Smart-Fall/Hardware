# SmartFall Wearable Fall Detection System

A comprehensive IoT solution for real-time fall detection using multi-sensor data fusion and emergency alert capabilities. Designed for ESP32 Feather V2 with simulation-first development approach using Wokwi.

## 🚀 Quick Start with Wokwi Simulation

### Method 1: VSCode with Wokwi Extension (Recommended)
1. **Install PlatformIO extension** in VSCode
2. **Install Wokwi extension** in VSCode
3. **Open project** in VSCode
4. **Build the project**: Press Ctrl+Shift+P → "PlatformIO: Build"
5. **Start Wokwi simulation**: Press F1 → "Wokwi: Start Simulator"

### Method 2: Online Wokwi Simulator
1. **Build firmware locally**: `pio run -e esp32dev`
2. **Open Wokwi simulator** at [https://wokwi.com](https://wokwi.com)
3. **Create new ESP32 project** and replace code with `src/main.cpp`
4. **Import the circuit** using `diagram.json`
5. **Upload firmware**: Use the generated `.bin` file from `.pio/build/esp32dev/`

### Key Simulation Components
- **ESP32 DevKit V1** (simulating ESP32 Feather V2)
- **MPU6050** (simulating BMI-323 6-axis IMU)
- **DHT22** (simulating BMP-280 pressure sensor)
- **2x Potentiometers** (simulating MAX30102 heart rate & FSR sensors)
- **Push Button** (SOS emergency trigger)
- **3x LEDs** (Alert system indicators: Audio/Haptic/Visual)
- **LCD1602** (Optional status display)

## 🎯 How to Test Fall Detection

### Simulating Fall Events
1. **Stage 1 - Free Fall**: Adjust MPU6050 readings to show <0.5g acceleration
2. **Stage 2 - Impact**: Create acceleration spike >3.0g within 1 second
3. **Stage 3 - Rotation**: Generate angular velocity >250°/s
4. **Stage 4 - Inactivity**: Maintain stable position ~1g for 2+ seconds
5. **Confidence Scoring**: System will calculate 105-point confidence score

### Manual Emergency Test
- **Press red SOS button** to trigger immediate emergency alert
- All alert LEDs will activate instantly
- System bypasses detection stages for manual activation

## 📊 System Architecture

### 5-Stage Fall Detection Algorithm
```
Stage 1: Free Fall Detection  → 25 points max
Stage 2: Impact Analysis      → 25 points max
Stage 3: Rotation Assessment  → 20 points max
Stage 4: Inactivity Check     → 20 points max
Stage 5: False Positive Filters → 15 points max
Total: 105 points possible
```

### Confidence Thresholds
- **≥80 points**: HIGH CONFIDENCE FALL → Immediate alert
- **70-79 points**: CONFIRMED FALL → 5-second delay alert
- **50-69 points**: POTENTIAL FALL → Enhanced monitoring
- **<50 points**: Continue monitoring or reset

### Hardware Abstraction
The project uses simulation-friendly component mapping:
- **Real Hardware**: BMI-323, BMP-280, MAX30102, FSR sensors
- **Wokwi Simulation**: MPU6050, DHT22, Potentiometers
- **Same Code Base**: Hardware abstraction layer enables seamless migration

## 🛠️ Development Commands

### PlatformIO Commands
```bash
# Build for Wokwi simulation
pio run -e esp32dev

# Build for real ESP32 Feather V2
pio run -e esp32featherv2

# Upload and monitor
pio run -t upload && pio device monitor

# Clean build
pio run -t clean
```

### Configuration Files
- **`platformio.ini`**: Build configuration for both simulation and hardware
- **`wokwi.toml`**: Wokwi project configuration and firmware paths
- **`wokwi-project.json`**: Wokwi circuit definition and component wiring
- **`src/utils/config.h`**: System constants and pin definitions

## 📁 Project Structure

```
SmartFall/
├── src/
│   ├── main.cpp                    # System coordinator and main loop
│   ├── sensors/                    # Hardware abstraction modules
│   │   ├── bmi323_sensor.cpp/.h    # IMU sensor (MPU6050 simulation)
│   │   ├── bmp280_sensor.cpp/.h    # Pressure sensor (DHT22 simulation)
│   │   ├── max30102_sensor.cpp/.h  # Heart rate (potentiometer simulation)
│   │   └── fsr_sensor.cpp/.h       # Force sensor (potentiometer simulation)
│   ├── detection/                  # Fall detection algorithm
│   │   ├── fall_detector.cpp/.h    # Multi-stage detection logic
│   │   └── confidence_scorer.cpp/.h # Confidence scoring system
│   └── utils/                      # Shared utilities
│       ├── config.h                # System constants and pin definitions
│       └── data_types.h            # Common data structures
├── platformio.ini                  # Build configuration
├── wokwi-project.json             # Wokwi simulation setup
└── CLAUDE.md                      # Claude Code integration guide
```

## 🔧 Key Features

### Multi-Sensor Data Fusion
- **6-axis IMU**: Acceleration and gyroscope data for motion analysis
- **Pressure Sensor**: Altitude change detection during falls
- **Heart Rate Monitor**: Physiological response validation
- **Force Sensor**: Impact detection and device attachment validation

### Real-Time Processing
- **100Hz sampling rate** for accurate motion capture
- **10-second detection window** with stage-based analysis
- **Memory-optimized** for ESP32's 8MB Flash / 2MB PSRAM constraints
- **Interrupt-driven** SOS button with <100ms response time

### Advanced Algorithm
- **Sequential stage detection** prevents false positives
- **Confidence-based scoring** with detailed breakdown
- **Hardware abstraction** enables simulation-first development
- **Tunable thresholds** for different user profiles

## 🚨 Alert System

### Visual Feedback (Simulation)
- **Yellow LED**: Audio alert indicator
- **Blue LED**: Haptic feedback indicator
- **Red LED**: Visual alert indicator
- **LCD Display**: System status and countdown

### Real Hardware Alerts
- **Audio**: PAM8302 amplifier + speaker with voice messages
- **Haptic**: DRV2605 driver + vibration motor patterns
- **Visual**: TFT display with emergency countdown

## 📡 Communication (Future Development)

### Bluetooth Low Energy
- Mobile app connectivity
- Real-time sensor data streaming
- Emergency contact notifications

### WiFi Communication
- Web server integration
- Cloud-based alert processing
- Remote monitoring dashboard

## 🧪 Testing and Validation

### Simulation Testing
- **Controlled sensor inputs** for reproducible test scenarios
- **Visual debugging** with LED indicators for each detection stage
- **Serial monitor output** with detailed algorithm progression
- **Parameter tuning** with immediate feedback

### Hardware Migration
- **Identical codebase** transitions from simulation to real hardware
- **Pin mapping updates** handled by configuration files
- **Library substitution** managed by PlatformIO environments
- **Calibration procedures** for real sensor characteristics

## 📈 Performance Characteristics

### Detection Accuracy
- **Free fall threshold**: <0.5g for ≥200ms
- **Impact threshold**: >3.0g within 1 second
- **Rotation threshold**: >250°/s angular velocity
- **Inactivity threshold**: ≥2 seconds stable position

### System Timing
- **Sensor sampling**: 10ms intervals (100Hz)
- **Detection response**: <2 seconds from fall to first alert
- **Alert timeout**: 30-second user response window
- **Processing overhead**: <5% CPU utilization on ESP32

## 🎓 Educational Value

### Algorithm Learning
- **Step-by-step detection logic** with clear stage progression
- **Confidence scoring mathematics** with detailed breakdowns
- **False positive mitigation** through multi-sensor validation
- **Real-time system design** patterns for embedded applications

### Hardware Integration
- **Sensor fusion techniques** combining multiple data sources
- **Interrupt handling** for time-critical events
- **Memory management** for resource-constrained environments
- **Power optimization** strategies for wearable devices

## 🤝 Development Team

This project supports **parallel development** with clear module boundaries:
- **Sensor Integration Team**: Hardware abstraction and sensor modules
- **Algorithm Team**: Fall detection logic and confidence scoring
- **Communication Team**: Bluetooth/WiFi and emergency protocols
- **System Integration Team**: Main coordination and testing

## 📄 License

MIT License - Feel free to use this project for educational and research purposes.
