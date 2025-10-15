# SmartFall

Complete fall detection system for ESP32 with modular sensor testing.

## Project Structure

```
SmartFall/
├── SmartFall.ino              # Main fall detection sketch
├── sketch.yaml                # Arduino CLI configuration
├── README.md                  # This file
│
├── sensors/                   # Sensor header files
│   ├── MPU6050_Sensor.h
│   ├── BMP280_Sensor.h
│   ├── MAX30102_Sensor.h
│   └── FSR_Sensor.h
│
├── detection/                 # Fall detection logic
│   ├── fall_detector.h/cpp
│   └── confidence_scorer.h/cpp
│
├── utils/                     # Configuration & data types
│   ├── config.h
│   └── data_types.h
│
├── *.cpp                      # Implementation files (for Arduino CLI)
│
└── Individual Sensor Test Modules:
    ├── MPU6050/              # MPU6050 test sketch
    ├── BMP280/               # BMP280 test sketch
    ├── MAX30102/             # MAX30102 test sketch
    └── FSR/                  # FSR test sketch
```

## Two Ways to Use This Project

1. **Test Individual Sensors**: Open any test sketch from sensor folders (e.g., `MPU6050/MPU6050_Test.ino`)
2. **Run Complete System**: Open `SmartFall.ino` for full fall detection

## Hardware Requirements

- **ESP32 HUZZAH32 Feather** (Adafruit)
- **MPU6050** - 6-axis IMU (I2C)
- **BMP280** - Barometric pressure sensor (I2C)
- **MAX30102** - Heart rate sensor (I2C, optional)
- **FSR** - Force sensitive resistor (Analog, optional)
- **Push button** - SOS emergency button
- **LEDs/Speaker** - Alert outputs

## Development Environment Setup

### Option 1: Arduino CLI (Recommended for Command Line)

#### Linux/Ubuntu Installation

```bash
# Download and install Arduino CLI
curl -fsSL https://raw.githubusercontent.com/arduino/arduino-cli/master/install.sh | sh

# Add to PATH (add to ~/.bashrc or ~/.zshrc for persistence)
export PATH=$PATH:$HOME/bin

# Verify installation
arduino-cli version

# Initialize configuration
arduino-cli config init

# Update board index
arduino-cli core update-index

# Install ESP32 board support
arduino-cli core install esp32:esp32

# Install required libraries
arduino-cli lib install "Adafruit MPU6050" "Adafruit BMP280 Library" \
  "SparkFun MAX3010x Pulse and Proximity Sensor Library" \
  "Adafruit Unified Sensor" "Adafruit DRV2605 Library" \
  "Adafruit GFX Library" "Adafruit SSD1306"
```

#### Windows PowerShell Installation

```powershell
# Download Arduino CLI installer
Invoke-WebRequest -Uri "https://downloads.arduino.cc/arduino-cli/arduino-cli_latest_Windows_64bit.zip" -OutFile "$env:TEMP\arduino-cli.zip"

# Extract to a directory (e.g., C:\arduino-cli)
Expand-Archive -Path "$env:TEMP\arduino-cli.zip" -DestinationPath "C:\arduino-cli"

# Add to PATH (run as Administrator)
[Environment]::SetEnvironmentVariable("Path", $env:Path + ";C:\arduino-cli", "Machine")

# Close and reopen PowerShell, then verify
arduino-cli version

# Initialize configuration
arduino-cli config init

# Update board index
arduino-cli core update-index

# Install ESP32 board support
arduino-cli core install esp32:esp32

# Install required libraries
arduino-cli lib install "Adafruit MPU6050"
arduino-cli lib install "Adafruit BMP280 Library"
arduino-cli lib install "SparkFun MAX3010x Pulse and Proximity Sensor Library"
arduino-cli lib install "Adafruit Unified Sensor"
arduino-cli lib install "Adafruit DRV2605 Library"
arduino-cli lib install "Adafruit GFX Library"
arduino-cli lib install "Adafruit SSD1306"
```

#### Arduino CLI Workflow

```bash
# Navigate to SmartFall directory
cd SmartFall

# Compile the sketch
arduino-cli compile --fqbn esp32:esp32:featheresp32 .

# List available ports
arduino-cli board list

# Upload to board (replace PORT)
# Linux: /dev/ttyUSB0 or /dev/ttyACM0
# Windows: COM3, COM4, etc.
arduino-cli upload -p PORT --fqbn esp32:esp32:featheresp32 .

# Open serial monitor (115200 baud)
arduino-cli monitor -p PORT -c baudrate=115200

# To exit serial monitor: Ctrl+C
```

**Port Configuration:**
- Edit `sketch.yaml` to set your default port
- Linux/macOS: `/dev/ttyUSB0` (or `/dev/ttyACM0`)
- Windows: `COM3` (check Device Manager for actual port)

**Linux Permissions Fix:**
```bash
# Add user to dialout group for serial port access
sudo usermod -a -G dialout $USER
# Log out and back in for changes to take effect
```

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

#### Required Libraries:
- **Adafruit MPU6050** (version 2.2.4 or later)
- **Adafruit Unified Sensor** (version 1.1.9 or later)
- **Adafruit BMP280 Library** (version 2.6.8 or later)
- **Adafruit BusIO** (automatically installed)

#### Optional Libraries (for full functionality):
- **SparkFun MAX3010x Pulse and Proximity Sensor** (version 1.1.2 or later)
- **Adafruit DRV2605 Library** (version 1.2.3 or later) - for haptic feedback
- **Adafruit GFX Library** (version 1.11.7 or later) - for display
- **Adafruit SSD1306** (version 2.5.7 or later) - for OLED display

#### 3. Board Configuration

1. **Select Board**: Tools → Board → ESP32 Arduino → **"Adafruit ESP32 Feather"**
2. **Upload Speed**: Tools → Upload Speed → **921600**
3. **Flash Frequency**: Tools → Flash Frequency → **80MHz**
4. **Partition Scheme**: Tools → Partition Scheme → **"Default 4MB with spiffs"**
5. **Port**: Tools → Port → Select your USB port
   - Linux: `/dev/ttyUSB0` or `/dev/ttyACM0`
   - Mac: `/dev/cu.usbserial-*`
   - Windows: `COM3` (or similar)

## Wiring Diagram

### I2C Sensors (Shared Bus)
All I2C devices connect to the same SDA/SCL pins:

| Sensor      | ESP32 Pin |
|-------------|-----------|
| SDA (All)   | GPIO 23   |
| SCL (All)   | GPIO 22   |
| VCC (All)   | 3.3V      |
| GND (All)   | GND       |

### Analog & Digital Pins

| Component          | ESP32 Pin | Type    |
|-------------------|-----------|---------|
| FSR Sensor        | A2        | Analog  |
| SOS Button        | GPIO 15   | Digital |
| Speaker/Buzzer    | GPIO 25   | Digital |
| Haptic Motor      | GPIO 26   | Digital |
| Visual Alert LED  | GPIO 27   | Digital |
| Battery Monitor   | A13       | Analog  |

## Quick Start

### Using Arduino CLI (Recommended)
```bash
cd SmartFall
arduino-cli compile --fqbn esp32:esp32:featheresp32 .
arduino-cli upload -p PORT --fqbn esp32:esp32:featheresp32 .
arduino-cli monitor -p PORT -c baudrate=115200
```

### Using Arduino IDE
1. Open `SmartFall.ino`
2. Select board: ESP32 Arduino → Adafruit ESP32 Feather
3. Select port (Tools → Port)
4. Click Upload (→)
5. Open Serial Monitor (Ctrl+Shift+M) at 115200 baud

## Serial Monitor Output

After uploading, you should see:
```
=== SmartFall Initialization ===

--- Initializing Sensors ---
✓ MPU6050 initialized
✓ BMP280 initialized
✓ MAX30102 initialized
✓ FSR initialized
✓ Fall detector initialized

=== SmartFall Ready ===
Monitoring for falls...
```

## Fall Detection Algorithm

The system uses a **5-stage confidence-based detection**:

1. **Stage 1**: Free Fall Detection (<0.5g)
2. **Stage 2**: Impact Analysis (>3.0g)
3. **Stage 3**: Rotation Assessment (>250°/s)
4. **Stage 4**: Inactivity Check (≥2 seconds)
5. **Stage 5**: Confidence Scoring (0-105 points)

### Confidence Thresholds:
- **≥80 points**: HIGH CONFIDENCE → Immediate alert
- **70-79 points**: CONFIRMED → 5-second delay alert
- **50-69 points**: POTENTIAL → Enhanced monitoring
- **<50 points**: Continue monitoring

## Testing

### Manual SOS Test
- Press the SOS button (GPIO 15)
- All alerts should activate immediately
- Release button to reset

### Simulated Fall Test
- Rapidly move/shake the device to trigger acceleration thresholds
- Monitor serial output for stage progression
- Verify alert activation on fall detection

## Troubleshooting

### Sensor Initialization Fails
- **Check wiring**: Verify I2C connections (SDA/SCL)
- **Check I2C addresses**: Run I2C scanner sketch
- **Power issues**: Ensure stable 3.3V power supply

### Upload Fails
- **Press BOOT button**: Hold during upload if needed
- **Check USB cable**: Try different cable
- **Driver issues**: Install CP2104 or CH340 USB drivers
- **Port permissions**: Linux users may need: `sudo usermod -a -G dialout $USER`

### No Serial Output
- Verify baud rate is **115200**
- Check USB connection
- Press RESET button on board

## Configuration

Edit `utils/config.h` to adjust:
- **Detection thresholds**: Sensitivity tuning
- **Pin assignments**: Custom wiring
- **Alert timing**: Countdown durations
- **Debug output**: Enable/disable verbose logging

## Project Structure

```
SmartFall_Arduino/
├── SmartFall_Arduino.ino    # Main sketch
├── sensors/
│   ├── mpu6050_sensor.*     # MPU6050 IMU
│   ├── bmp280_sensor.*      # BMP280 pressure
│   ├── max30102_sensor.*    # MAX30102 heart rate
│   └── fsr_sensor.*         # Force sensor
├── detection/
│   ├── fall_detector.*      # Fall detection logic
│   └── confidence_scorer.*  # Confidence scoring
└── utils/
    ├── config.h             # System configuration
    └── data_types.h         # Data structures
```

## License

MIT License - See LICENSE file for details

## Support

For issues or questions, refer to the main project documentation or open an issue on the project repository.