# SmartFall Wearable Fall Detection System

A comprehensive IoT solution for real-time fall detection using multi-sensor data fusion and emergency alert capabilities for ESP32 HUZZAH32 Feather.

## 🚀 Quick Start

### Arduino CLI Setup (Recommended)

#### Linux/Ubuntu Installation
```bash
# Install Arduino CLI
curl -fsSL https://raw.githubusercontent.com/arduino/arduino-cli/master/install.sh | sh
export PATH=$PATH:$HOME/bin

# Setup ESP32 and libraries
arduino-cli config init
arduino-cli core update-index
arduino-cli core install esp32:esp32
arduino-cli lib install "Adafruit MPU6050" "Adafruit BMP280 Library" \
  "SparkFun MAX3010x Pulse and Proximity Sensor Library" \
  "Adafruit Unified Sensor" "Adafruit DRV2605 Library" \
  "Adafruit GFX Library" "Adafruit SSD1306"

# Fix serial port permissions
sudo usermod -a -G dialout $USER
# Log out and back in
```

#### Windows PowerShell Installation
```powershell
# Download and install
Invoke-WebRequest -Uri "https://downloads.arduino.cc/arduino-cli/arduino-cli_latest_Windows_64bit.zip" -OutFile "$env:TEMP\arduino-cli.zip"
Expand-Archive -Path "$env:TEMP\arduino-cli.zip" -DestinationPath "C:\arduino-cli"
[Environment]::SetEnvironmentVariable("Path", $env:Path + ";C:\arduino-cli", "Machine")

# Restart PowerShell, then setup
arduino-cli config init
arduino-cli core update-index
arduino-cli core install esp32:esp32
arduino-cli lib install "Adafruit MPU6050"
arduino-cli lib install "Adafruit BMP280 Library"
arduino-cli lib install "SparkFun MAX3010x Pulse and Proximity Sensor Library"
arduino-cli lib install "Adafruit Unified Sensor"
```

### Build and Upload
```bash
cd SmartFall

# Compile
arduino-cli compile --fqbn esp32:esp32:featheresp32 .

# List available ports
arduino-cli board list

# Upload (Linux: /dev/ttyUSB0, Windows: COM3)
arduino-cli upload -p PORT --fqbn esp32:esp32:featheresp32 .

# Monitor serial output (115200 baud)
arduino-cli monitor -p PORT -c baudrate=115200
```

**Port Configuration:**
- Edit `SmartFall/sketch.yaml` to set your default port
- Linux: `/dev/ttyUSB0` or `/dev/ttyACM0`
- Windows: `COM3` (check Device Manager)

## 🎯 Hardware Components

### Required Sensors
- **ESP32 HUZZAH32 Feather** (Adafruit)
- **MPU6050** - 6-axis IMU (I2C)
- **BMP280** - Barometric pressure sensor (I2C)

### Optional Components
- **MAX30102** - Heart rate sensor (I2C)
- **FSR** - Force sensitive resistor (Analog)
- **Push button** - SOS emergency button (GPIO 15)
- **LEDs/Speaker** - Alert outputs

### I2C Wiring (All sensors share same bus)
| Sensor | VCC | GND | SDA | SCL |
|--------|-----|-----|-----|-----|
| MPU6050 | 3.3V | GND | GPIO 23 | GPIO 22 |
| BMP280 | 3.3V | GND | GPIO 23 | GPIO 22 |
| MAX30102 | 3.3V | GND | GPIO 23 | GPIO 22 |

## 🎯 How to Test Fall Detection

### Real Hardware Testing
1. **Manual Testing**: Rapidly move/shake the device to trigger acceleration thresholds
2. **Monitor Serial Output**: Watch for stage progression in serial monitor
3. **SOS Button**: Press GPIO 15 button for immediate emergency alert

### Individual Sensor Testing
Test each sensor independently using modules in `SmartFall/` directory:
```bash
# Test MPU6050 only
arduino-cli compile --fqbn esp32:esp32:featheresp32 SmartFall/MPU6050
arduino-cli upload -p PORT --fqbn esp32:esp32:featheresp32 SmartFall/MPU6050

# Test BMP280 only
arduino-cli compile --fqbn esp32:esp32:featheresp32 SmartFall/BMP280
arduino-cli upload -p PORT --fqbn esp32:esp32:featheresp32 SmartFall/BMP280
```

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

### Modular Design
- **Individual sensor testing** with standalone test sketches
- **Unified main sketch** for complete fall detection system
- **Hardware abstraction** for easy sensor substitution
- **Configurable thresholds** via `utils/config.h`

## 🛠️ Development Tools

### Arduino CLI Commands
```bash
# Compile main sketch
arduino-cli compile --fqbn esp32:esp32:featheresp32 SmartFall

# Upload to board
arduino-cli upload -p PORT --fqbn esp32:esp32:featheresp32 SmartFall

# Serial monitor
arduino-cli monitor -p PORT -c baudrate=115200

# List connected boards
arduino-cli board list
```

### PlatformIO (Alternative)
```bash
# Build for ESP32 HUZZAH32
pio run -e huzzah32

# Upload and monitor
pio run -t upload && pio device monitor
```

### Configuration Files
- **`SmartFall/sketch.yaml`**: Arduino CLI project configuration
- **`platformio.ini`**: PlatformIO build configuration
- **`SmartFall/utils/config.h`**: System constants and pin definitions
- **`partitions.csv`**: ESP32 flash partition table

## 📁 Project Structure

```
Hardware/
├── SmartFall/                      # Main Arduino sketch directory
│   ├── SmartFall.ino              # Main fall detection sketch
│   ├── sketch.yaml                # Arduino CLI configuration
│   ├── *.cpp                      # Implementation files (Arduino requirement)
│   │
│   ├── sensors/                   # Sensor header files
│   │   ├── MPU6050_Sensor.h
│   │   ├── BMP280_Sensor.h
│   │   ├── MAX30102_Sensor.h
│   │   └── FSR_Sensor.h
│   │
│   ├── detection/                 # Fall detection algorithm
│   │   ├── fall_detector.h/cpp
│   │   └── confidence_scorer.h/cpp
│   │
│   ├── utils/                     # Configuration
│   │   ├── config.h               # Pin definitions and thresholds
│   │   └── data_types.h           # Data structures
│   │
│   └── Individual Test Modules:
│       ├── MPU6050/               # MPU6050 standalone test
│       ├── BMP280/                # BMP280 standalone test
│       ├── MAX30102/              # MAX30102 standalone test
│       └── FSR/                   # FSR standalone test
│
├── README.md                       # This file
├── FallDetectionAlgorithm.md       # Algorithm documentation
├── Code Structure and Development Guidelines.md
├── platformio.ini                  # PlatformIO configuration (optional)
└── partitions.csv                  # ESP32 partition table
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
