# SmartFall Wearable Fall Detection System

A comprehensive IoT solution for real-time fall detection using multi-sensor data fusion and emergency alert capabilities for ESP32 HUZZAH32 Feather.

---

## 📋 Table of Contents

1. [Quick Start](#-quick-start)
2. [Hardware Components](#-hardware-components)
3. [System Architecture](#-system-architecture)
4. [Project Structure](#-project-structure)
5. [Development Setup](#-development-setup)
6. [Wiring Guide](#-wiring-guide)
7. [Testing](#-testing)
8. [Communication System](#-communication-system)
9. [Audio System](#-audio-system)
10. [Configuration](#-configuration)
11. [Troubleshooting](#-troubleshooting)
12. [Advanced Topics](#-advanced-topics)

---

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
  "Adafruit Unified Sensor" "ArduinoJson"

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
arduino-cli lib install "Adafruit MPU6050" "Adafruit BMP280 Library" \
  "SparkFun MAX3010x Pulse and Proximity Sensor Library" \
  "Adafruit Unified Sensor" "ArduinoJson"
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

---

## 🎯 Hardware Components

### Required Components
- **ESP32 HUZZAH32 Feather** (Adafruit) - Main microcontroller
- **MPU6050** - 6-axis IMU (accelerometer + gyroscope) - I2C
- **BMP280** - Barometric pressure sensor - I2C
- **MAX30102** - Heart rate sensor - I2C
- **FSR** - Force sensitive resistor - Analog
- **PAM8302** - 2.5W Class D audio amplifier
- **Speaker** - 4Ω or 8Ω, 0.5W-1W
- **Push button** - SOS emergency button (GPIO 15)
- **LEDs** - Visual alert indicators

### Optional Components
- **Haptic motor** - Vibration feedback
- **OLED display** - System status display
- **Battery** - LiPo battery for portable operation

---

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

For detailed algorithm documentation, see [FallDetectionAlgorithm.md](FallDetectionAlgorithm.md).

### Modular Design
- **Individual sensor testing** with standalone test sketches in `tests/`
- **Unified main sketch** (`SmartFall.ino`) for complete fall detection system
- **Hardware abstraction** for easy sensor substitution
- **Configurable thresholds** via `utils/config.h`

---

## 📁 Project Structure

```
Hardware/
├── README.md                       # This file - complete documentation
├── FallDetectionAlgorithm.md       # Detailed algorithm specification
├── platformio.ini                  # PlatformIO configuration (optional)
├── partitions.csv                  # ESP32 partition table
│
└── SmartFall/                      # Main Arduino sketch directory
    ├── SmartFall.ino              # MAIN COMPLETE SKETCH (production-ready)
    ├── sketch.yaml                # Arduino CLI configuration
    ├── *.cpp                      # Implementation files (Arduino requirement)
    │
    ├── sensors/                   # Sensor drivers (shared by main sketch)
    │   ├── MPU6050_Sensor.h/cpp
    │   ├── BMP280_Sensor.h/cpp
    │   ├── MAX30102_Sensor.h/cpp
    │   └── FSR_Sensor.h/cpp
    │
    ├── detection/                 # Fall detection algorithm
    │   ├── fall_detector.h/cpp
    │   └── confidence_scorer.h/cpp
    │
    ├── communication/             # WiFi + BLE modules
    │   ├── WiFi_Manager.h/cpp
    │   ├── BLE_Server.h/cpp
    │   └── Emergency_Comms.h/cpp
    │
    ├── audio/                     # Audio system (PAM8302)
    │   └── Audio_Manager.h/cpp
    │
    ├── utils/                     # Configuration and data types
    │   ├── config.h               # Pin definitions, WiFi, BLE, Audio settings
    │   └── data_types.h           # Data structures
    │
    └── tests/                     # Individual component tests
        ├── MPU6050/              # IMU sensor test
        ├── BMP280/               # Pressure sensor test
        ├── MAX30102/             # Heart rate sensor test
        ├── FSR/                  # Force sensor test
        ├── MPU_BMP/              # Combined sensors test
        ├── WiFi/                 # WiFi connectivity test
        ├── BLE/                  # Bluetooth test
        └── Audio/                # Audio system test
```

### Main Sketch vs Test Modules

- **For Production**: Use `SmartFall/SmartFall.ino` (complete system with all features)
- **For Testing**: Use `SmartFall/tests/[Component]/` (individual component tests)
- **For Configuration**: Edit `SmartFall/utils/config.h`

Test modules are self-contained with their own driver copies for isolated component testing and debugging.

---

## 🛠️ Development Setup

### Option 1: Arduino CLI (Command Line - Recommended)

Already covered in [Quick Start](#-quick-start) section above.

### Option 2: Arduino IDE Setup

#### 1. Install ESP32 Board Support

1. Open Arduino IDE
2. Go to **File → Preferences**
3. Add to "Additional Board Manager URLs":
   ```
   https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
   ```
4. Go to **Tools → Board → Boards Manager**
5. Search for "ESP32" and install **"esp32 by Espressif Systems"**

#### 2. Install Required Libraries

Go to **Tools → Manage Libraries** and install:

- **Adafruit MPU6050** (version 2.2.4 or later)
- **Adafruit Unified Sensor** (version 1.1.9 or later)
- **Adafruit BMP280 Library** (version 2.6.8 or later)
- **SparkFun MAX3010x Pulse and Proximity Sensor** (version 1.1.2 or later)
- **ArduinoJson** (version 6.21.3 or later)
- **Adafruit BusIO** (automatically installed)

#### 3. Board Configuration

1. **Select Board**: Tools → Board → ESP32 Arduino → **"Adafruit ESP32 Feather"**
2. **Upload Speed**: Tools → Upload Speed → **921600**
3. **Flash Frequency**: Tools → Flash Frequency → **80MHz**
4. **Partition Scheme**: Tools → Partition Scheme → **"Default 4MB with spiffs"**
5. **Port**: Tools → Port → Select your USB port
   - Linux: `/dev/ttyUSB0` or `/dev/ttyACM0`
   - Mac: `/dev/cu.usbserial-*`
   - Windows: `COM3` (or similar)

### Option 3: PlatformIO (Alternative)

```bash
# Build for ESP32 HUZZAH32
pio run -e huzzah32

# Upload and monitor
pio run -t upload && pio device monitor
```

---

## 🔌 Wiring Guide

### I2C Sensors (Shared Bus)

All I2C devices connect to the same SDA/SCL pins:

| Sensor   | VCC  | GND | SDA     | SCL     |
|----------|------|-----|---------|---------|
| MPU6050  | 3.3V | GND | GPIO 23 | GPIO 22 |
| BMP280   | 3.3V | GND | GPIO 23 | GPIO 22 |
| MAX30102 | 3.3V | GND | GPIO 23 | GPIO 22 |

### Analog & Digital Pins

| Component          | ESP32 Pin | Type    | Description |
|-------------------|-----------|---------|-------------|
| FSR Sensor        | A2        | Analog  | Force sensor |
| SOS Button        | GPIO 15   | Digital | Emergency button (INPUT_PULLUP) |
| Speaker (PAM8302) | GPIO 25   | PWM     | Audio output |
| Haptic Motor      | GPIO 26   | Digital | Vibration feedback |
| Visual Alert LED  | GPIO 27   | Digital | LED indicator |
| Battery Monitor   | A13       | Analog  | Battery voltage |

### PAM8302 Audio Amplifier Wiring

```
ESP32 HUZZAH32          PAM8302 Amplifier         Speaker
    Feather

GPIO 25 (PWM) ───────── A+ (Audio Input+)
GND ────────────────── A- (Audio Input-)
                       GND (Ground) ──────────── Speaker (-)
3.3V ──────────────── VIN (Power)
                       OUT+ ─────────────────── Speaker (+)
                       OUT- ─────────────────── Speaker (-)
```

**Speaker Recommendations:**
- **8Ω 0.5W**: Clear audio, low power
- **8Ω 1W**: Louder, better bass
- **4Ω 3W**: Maximum loudness (higher current draw)

---

## 🧪 Testing

### Quick Test: Main System

1. **Upload Main Sketch**
   ```bash
   cd SmartFall
   arduino-cli compile --fqbn esp32:esp32:featheresp32 .
   arduino-cli upload -p PORT --fqbn esp32:esp32:featheresp32 .
   arduino-cli monitor -p PORT -c baudrate=115200
   ```

2. **Expected Serial Output:**
   ```
   ========================================
         SmartFall Detection System
      Complete with Audio & Communication
   ========================================

   --- Initializing Sensors ---
   ✓ MPU6050 initialized
   ✓ BMP280 initialized
   ✓ MAX30102 initialized
   ✓ FSR initialized
   ✓ Fall detector initialized

   --- Initializing Communication ---
   ✓ WiFi connected
   ✓ BLE server started
   ✓ Emergency communication system ready

   ========================================
          SmartFall Ready!
   ========================================
   Monitoring for falls...
   ```

### Individual Component Testing

Test each component individually before running the complete system:

```bash
# Test MPU6050 (IMU)
cd SmartFall/tests/MPU6050
arduino-cli compile --fqbn esp32:esp32:featheresp32 .
arduino-cli upload -p PORT --fqbn esp32:esp32:featheresp32 .

# Test BMP280 (Pressure)
cd ../BMP280
arduino-cli compile --fqbn esp32:esp32:featheresp32 .
arduino-cli upload -p PORT --fqbn esp32:esp32:featheresp32 .

# Test MAX30102 (Heart Rate)
cd ../MAX30102
arduino-cli compile --fqbn esp32:esp32:featheresp32 .
arduino-cli upload -p PORT --fqbn esp32:esp32:featheresp32 .

# Test FSR (Force Sensor)
cd ../FSR
arduino-cli compile --fqbn esp32:esp32:featheresp32 .
arduino-cli upload -p PORT --fqbn esp32:esp32:featheresp32 .

# Test WiFi
cd ../WiFi
arduino-cli compile --fqbn esp32:esp32:featheresp32 .
arduino-cli upload -p PORT --fqbn esp32:esp32:featheresp32 .

# Test BLE
cd ../BLE
arduino-cli compile --fqbn esp32:esp32:featheresp32 .
arduino-cli upload -p PORT --fqbn esp32:esp32:featheresp32 .

# Test Audio
cd ../Audio
arduino-cli compile --fqbn esp32:esp32:featheresp32 .
arduino-cli upload -p PORT --fqbn esp32:esp32:featheresp32 .
```

### Fall Detection Testing

#### Method 1: Manual SOS Test
- Press the SOS button (GPIO 15)
- All alerts should activate immediately
- Release button to reset

#### Method 2: Simulated Fall Test
- Rapidly move/shake the device to trigger acceleration thresholds
- Monitor serial output for stage progression
- Verify alert activation on fall detection

#### Method 3: Controlled Testing
- Place device in controlled environment
- Trigger specific detection stages
- Observe confidence score buildup

---

## 📡 Communication System

SmartFall uses the ESP32's built-in wireless capabilities for dual-protocol emergency alerts.

### Communication Architecture

```
SmartFall Device (ESP32)
    ├── WiFi Manager → HTTP/HTTPS → Web Server → SMS/Email/Push
    └── BLE Server → Bluetooth → Mobile App → Local Notifications
```

### WiFi Configuration

#### Step 1: Update Configuration

Edit `SmartFall/utils/config.h`:

```cpp
// WiFi Configuration
#define WIFI_SSID                  "YourNetworkName"
#define WIFI_PASSWORD              "YourPassword"
#define WIFI_TIMEOUT_MS            10000
#define WIFI_RECONNECT_INTERVAL_MS 30000

// Server Configuration
#define SERVER_URL                 "http://your-server.com"
#define SERVER_PORT                80
```

#### Step 2: WiFi Features

- **Automatic connection** during startup
- **Auto-reconnect** every 30 seconds if disconnected
- **HTTP/HTTPS** alert transmission
- **JSON payload** format

#### Step 3: Server API Endpoints

Your server should implement these endpoints:

##### POST /api/emergency
Receives emergency fall alerts.

**Request Body:**
```json
{
  "timestamp": 123456789,
  "confidence_score": 85,
  "confidence_level": 4,
  "battery_level": 78.5,
  "sos_triggered": false,
  "device_id": "SF-AABBCCDDEEFF",
  "sensor_history": [...]
}
```

**Expected Response:**
```json
{
  "status": "received",
  "alert_id": "unique-alert-identifier"
}
```

##### POST /api/status
Receives periodic status updates (every minute).

##### POST /api/sensor
Receives real-time sensor data (if streaming enabled).

#### Example Node.js Server

```javascript
const express = require('express');
const app = express();
app.use(express.json());

app.post('/api/emergency', (req, res) => {
  console.log('Emergency Alert Received:');
  console.log(JSON.stringify(req.body, null, 2));

  // Send SMS/Email/Push notifications here
  // sendEmergencyNotifications(req.body);

  res.json({ status: 'received', alert_id: Date.now().toString() });
});

app.listen(80, () => {
  console.log('SmartFall server listening on port 80');
});
```

### BLE (Bluetooth Low Energy) Configuration

#### Step 1: BLE Settings

Edit `SmartFall/utils/config.h`:

```cpp
// BLE Configuration
#define BLE_DEVICE_NAME            "SmartFall"
#define BLE_STREAMING_INTERVAL_MS  1000
```

#### Step 2: BLE Service Structure

**Service UUID:** `4fafc201-1fb5-459e-8fcc-c5c9c331914b`

**Characteristics:**

| Characteristic | UUID | Properties | Purpose |
|----------------|------|------------|---------|
| Emergency Alert | `beb5483e-...` | Notify | Emergency notifications |
| Sensor Data | `beb5483f-...` | Notify | Real-time sensor streaming |
| Status | `beb54840-...` | Read, Notify | Device status |
| Command | `beb54841-...` | Write | App commands |
| Config | `beb54842-...` | Read, Write | Configuration |

#### Step 3: BLE Commands

Mobile apps can send commands via the Command characteristic:

| Command | Value | Description |
|---------|-------|-------------|
| Cancel Alert | `0x01` | Cancel ongoing emergency alert |
| Test Alert | `0x02` | Trigger test alert |
| Get Status | `0x03` | Request device status |
| Start Streaming | `0x05` | Enable sensor data streaming |
| Stop Streaming | `0x06` | Disable sensor data streaming |

#### Testing BLE

1. Use a BLE scanner app:
   - **iOS**: LightBlue
   - **Android**: nRF Connect

2. Scan for "SmartFall" device
3. Connect and explore services
4. Subscribe to Emergency characteristic
5. Trigger a test alert (press SOS button)

### Dual-Protocol Emergency Alerts

- **Redundant transmission** via both WiFi and BLE
- **Automatic retry** on failed transmissions (up to 3 attempts)
- **Priority-based routing** (WiFi preferred for cloud, BLE for mobile)
- **Offline queueing** for transmission when connection restored

---

## 🔊 Audio System

SmartFall uses the **Adafruit PAM8302 Mono 2.5W Class D Audio Amplifier** for audio feedback.

### Audio Features

#### 1. Pre-Defined Alert Patterns

```cpp
// Single patterns
audioManager.playPattern(ALERT_PATTERN_SINGLE_BEEP);
audioManager.playPattern(ALERT_PATTERN_DOUBLE_BEEP);
audioManager.playPattern(ALERT_PATTERN_TRIPLE_BEEP);

// Repeating patterns
audioManager.playPattern(ALERT_PATTERN_URGENT, 3);  // Play 3 times

// Available patterns:
// - ALERT_PATTERN_SINGLE_BEEP
// - ALERT_PATTERN_DOUBLE_BEEP
// - ALERT_PATTERN_TRIPLE_BEEP
// - ALERT_PATTERN_SIREN
// - ALERT_PATTERN_URGENT
// - ALERT_PATTERN_SOS (Morse code: ... --- ...)
// - ALERT_PATTERN_CONFIRMATION
// - ALERT_PATTERN_ERROR
// - ALERT_PATTERN_WARNING
```

#### 2. Voice-Like Alerts

These sequences use tone patterns to mimic speech:

```cpp
// Fall detected announcement
audioManager.playVoiceAlert(VOICE_ALERT_FALL_DETECTED);

// User guidance
audioManager.playVoiceAlert(VOICE_ALERT_PRESS_BUTTON);

// Status notifications
audioManager.playVoiceAlert(VOICE_ALERT_SYSTEM_READY);
audioManager.playVoiceAlert(VOICE_ALERT_CALLING_HELP);
audioManager.playVoiceAlert(VOICE_ALERT_HELP_SENT);

// Warnings
audioManager.playVoiceAlert(VOICE_ALERT_LOW_BATTERY);
audioManager.playVoiceAlert(VOICE_ALERT_CONNECTION_LOST);
```

#### 3. Special Sequences

```cpp
// Emergency sequences
audioManager.playFallDetectedSequence();
audioManager.playSOSSequence();  // SOS in Morse code (... --- ...)

// System feedback
audioManager.playConfirmationTone();  // Ascending success tone
audioManager.playErrorTone();         // Descending error tone
audioManager.playWarningTone();       // Alternating warning

// Startup melody
audioManager.playStartupMelody();
```

#### 4. Volume Control

```cpp
// Set volume (0-100)
audioManager.setVolume(80);  // Default 80%

// Get current volume
uint8_t volume = audioManager.getVolume();
```

### Audio Configuration

Edit `SmartFall/utils/config.h`:

```cpp
// Audio Configuration (PAM8302 Amplifier)
#define AUDIO_DEFAULT_VOLUME       80     // 0-100, default volume level
#define AUDIO_PWM_CHANNEL          0      // ESP32 PWM channel
#define AUDIO_PWM_FREQUENCY        5000   // Base PWM frequency (Hz)
#define AUDIO_PWM_RESOLUTION       8      // PWM resolution (bits)
#define AUDIO_ENABLE_VOICE_ALERTS  true   // Enable voice-like alert sequences
```

### Testing Audio System

Upload the audio test module:

```bash
cd SmartFall/tests/Audio
arduino-cli compile --fqbn esp32:esp32:featheresp32 .
arduino-cli upload -p PORT --fqbn esp32:esp32:featheresp32 .
```

The test will automatically play through all audio features:
- Startup melody
- Volume control test (25%, 50%, 75%, 100%)
- Frequency sweep (500Hz - 2000Hz)
- All alert patterns
- SOS Morse code
- Voice-like alert sequences

### Power Consumption

- **Idle (no audio)**: ~5mA
- **Low volume (20%)**: ~50mA
- **Medium volume (50%)**: ~150mA
- **High volume (80%)**: ~300mA
- **Maximum volume (100%)**: ~500mA

---

## ⚙️ Configuration

All system configuration is centralized in `SmartFall/utils/config.h`.

### Pin Definitions

```cpp
// I2C Pins (shared by all I2C sensors)
#define MPU6050_SDA_PIN            23
#define MPU6050_SCL_PIN            22

// Analog Pins
#define FSR_ANALOG_PIN             A2
#define BATTERY_SENSE_PIN          A13

// Digital Pins
#define SOS_BUTTON_PIN             15
#define SPEAKER_PIN                25
#define HAPTIC_PIN                 26
#define VISUAL_ALERT_PIN           27
```

### Fall Detection Thresholds

```cpp
// Algorithm thresholds
#define FREEFALL_THRESHOLD_G       0.5f    // Free fall threshold (g)
#define IMPACT_THRESHOLD_G         3.0f    // Impact threshold (g)
#define ROTATION_THRESHOLD_DPS     250.0f  // Rotation threshold (°/s)
#define INACTIVITY_THRESHOLD_MS    2000    // Inactivity duration (ms)

// Confidence scoring
#define MAX_CONFIDENCE_SCORE       105
#define HIGH_CONFIDENCE_THRESHOLD  80
#define CONFIRMED_THRESHOLD        70
#define POTENTIAL_THRESHOLD        50
#define SUSPICIOUS_THRESHOLD       30
```

### Timing Constants

```cpp
// System timing
#define SENSOR_SAMPLE_RATE_HZ      100     // 100Hz sampling
#define DETECTION_WINDOW_MS        10000   // 10-second window
#define ALERT_TIMEOUT_MS           30000   // 30-second countdown
#define COUNTDOWN_DURATION_S       30      // User response time

// Loop timing
#define MAIN_LOOP_DELAY_MS         10      // 100Hz main loop
#define SENSOR_READ_INTERVAL_MS    10      // 100Hz sensor reading
#define HEARTBEAT_INTERVAL_MS      1000    // Status LED blink
```

### Debug Settings

```cpp
// Debug settings
#define DEBUG_SENSOR_DATA          false   // Print sensor data
#define DEBUG_ALGORITHM_STEPS      true    // Print algorithm steps
#define DEBUG_COMMUNICATION        true    // Print communication debug
#define SERIAL_BAUD_RATE          115200  // Serial baud rate
```

---

## 🔧 Troubleshooting

### Sensor Initialization Fails

**Problem:** "ERROR: Failed to initialize [SENSOR]!"

**Solutions:**
1. **Check wiring**: Verify I2C connections (SDA/SCL)
2. **Check I2C addresses**: Run I2C scanner sketch
3. **Power issues**: Ensure stable 3.3V power supply
4. **Test individually**: Use component test modules in `tests/`

### Upload Fails

**Problem:** Cannot upload sketch to ESP32

**Solutions:**
1. **Press BOOT button**: Hold during upload if needed
2. **Check USB cable**: Try different cable (data cable, not charge-only)
3. **Driver issues**: Install CP2104 or CH340 USB drivers
4. **Port permissions** (Linux):
   ```bash
   sudo usermod -a -G dialout $USER
   # Log out and back in
   ```

### No Serial Output

**Problem:** Serial monitor shows nothing

**Solutions:**
1. Verify baud rate is **115200**
2. Check USB connection
3. Press RESET button on board
4. Try different USB port

### WiFi Connection Fails

**Problem:** "WiFi connection failed"

**Solutions:**
1. Verify SSID and password in `config.h`
2. Check if network is **2.4GHz** (ESP32 doesn't support 5GHz)
3. Ensure ESP32 is within range of router
4. Check if MAC address is allowed on network

### BLE Not Discoverable

**Problem:** Cannot find "SmartFall" device

**Solutions:**
1. Verify BLE is initialized (check serial output)
2. Move phone closer to ESP32
3. Restart both devices
4. Ensure no other BLE code is running

### No Audio Output

**Problem:** PAM8302 amplifier produces no sound

**Solutions:**
1. **Check wiring**:
   - GPIO 25 → A+ on PAM8302
   - GND → A- and GND on PAM8302
   - 3.3V → VIN on PAM8302
2. **Check speaker**: Verify 4Ω or 8Ω speaker is connected
3. **Check volume**: `audioManager.setVolume(100);`
4. **Test speaker**: Connect to another audio source

### Distorted Audio

**Problem:** Audio sounds distorted or crackling

**Solutions:**
1. Lower volume: `audioManager.setVolume(50);`
2. Add power filtering: 100µF capacitor between VIN and GND on PAM8302
3. Use separate power supply if ESP32 is power-constrained
4. Use 8Ω speaker instead of 4Ω for cleaner audio

---

## 🎓 Advanced Topics

### Multi-Sensor Data Fusion
- **6-axis IMU**: Acceleration and gyroscope data for motion analysis
- **Pressure Sensor**: Altitude change detection during falls
- **Heart Rate Monitor**: Physiological response validation
- **Force Sensor**: Impact detection and device attachment validation

### Real-Time Processing
- **100Hz sampling rate** for accurate motion capture
- **10-second detection window** with stage-based analysis
- **Memory-optimized** for ESP32's 4MB Flash / 520KB SRAM
- **Interrupt-driven** SOS button with <100ms response time

### Advanced Algorithm Features
- **Sequential stage detection** prevents false positives
- **Confidence-based scoring** with detailed breakdown
- **Hardware abstraction** enables simulation-first development
- **Tunable thresholds** for different user profiles

### Performance Characteristics

#### Detection Accuracy
- **Free fall threshold**: <0.5g for ≥200ms
- **Impact threshold**: >3.0g within 1 second
- **Rotation threshold**: >250°/s angular velocity
- **Inactivity threshold**: ≥2 seconds stable position

#### System Timing
- **Sensor sampling**: 10ms intervals (100Hz)
- **Detection response**: <2 seconds from fall to first alert
- **Alert timeout**: 30-second user response window
- **Processing overhead**: <5% CPU utilization on ESP32

### Development Team Support

This project supports **parallel development** with clear module boundaries:
- **Sensor Integration Team**: Hardware abstraction and sensor modules
- **Algorithm Team**: Fall detection logic and confidence scoring
- **Communication Team**: Bluetooth/WiFi and emergency protocols
- **System Integration Team**: Main coordination and testing

---

## 📄 License

MIT License - Feel free to use this project for educational and research purposes.

---

## 📚 Additional Documentation

- **Algorithm Details**: See [FallDetectionAlgorithm.md](FallDetectionAlgorithm.md) for complete algorithm specification
- **Test Modules**: See `SmartFall/tests/` for individual component tests
- **Configuration Reference**: See `SmartFall/utils/config.h` for all configurable parameters

---

## 🤝 Support

For issues or questions:
1. Check this README first
2. Review [FallDetectionAlgorithm.md](FallDetectionAlgorithm.md) for algorithm questions
3. Test individual components using test modules
4. Open an issue on the project repository

---

**SmartFall** - Comprehensive fall detection for enhanced safety and peace of mind.
