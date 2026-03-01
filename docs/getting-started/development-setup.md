# Development Setup

Complete guide for setting up your development environment for SmartFall firmware development.

## IDE Setup (Choose One)

=== "Arduino IDE"

    ### 1. Install Arduino IDE

    Download from [arduino.cc](https://www.arduino.cc/en/software) (version 1.8.x or 2.0+)

    ### 2. Add ESP32 Board Support

    1. **Preferences**: File → Preferences
    2. Add to "Additional Board Manager URLs":
       ```
       https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
       ```
    3. **Boards Manager**: Tools → Board → Boards Manager
    4. Search "ESP32" and install **"esp32 by Espressif Systems"** (v2.0.0+)

    ### 3. Install All Required Libraries

    **Tools → Manage Libraries** and install each:

    - **Adafruit MPU6050** v2.2.4+
    - **Adafruit Unified Sensor** v1.1.9+
    - **Adafruit BMP280 Library** v2.6.8+
    - **SparkFun MAX3010x Pulse and Proximity Sensor** v1.1.2+
    - **DFRobot_BloodOxygen_S** v1.0.0+
    - **DFRobot_RTU** v1.0.6+
    - **ArduinoJson** v6.21.3+
    - **Adafruit BusIO** (auto-installed)

    ### 4. Configure Board Settings

    #### For ESP32 Feather V2:
    - **Board**: Tools → Board → ESP32 Arduino → **Adafruit Feather ESP32 V2**
    - **Upload Speed**: **921600**
    - **Flash Frequency**: **80MHz**
    - **Partition Scheme**: **8M with spiffs**
    - **PSRAM**: **Enabled** (takes advantage of 2MB PSRAM)
    - **Port**: Select your USB port

    #### For ESP32 HUZZAH32:
    - **Board**: Tools → Board → ESP32 Arduino → **Adafruit ESP32 Feather**
    - **Upload Speed**: **921600**
    - **Flash Frequency**: **80MHz**
    - **Partition Scheme**: **Default 4MB with spiffs**
    - **Port**: Select your USB port

=== "PlatformIO (VS Code)"

    ### 1. Install VS Code & PlatformIO

    1. Download [Visual Studio Code](https://code.visualstudio.com/)
    2. Install **PlatformIO IDE for VSCode** extension
    3. Reload VS Code

    ### 2. Open SmartFall Project

    ```bash
    cd Hardware
    code .
    ```

    PlatformIO auto-discovers project configuration from `platformio.ini`.

    > **Project layout note:** The sketch lives in `Hardware/SmartFall/`. The
    > `platformio.ini` includes `src_dir = SmartFall` and
    > `build_src_filter = +<*> -<Tests/> -<SmartFall/> -<Utils/>` so that test
    > sub-sketches and utility copies are excluded from the main build.

    ### 3. Key platformio.ini settings

    ```ini
    [platformio]
    src_dir = SmartFall          ; sketch folder inside Hardware/

    [env:feather_esp32_v2]
    platform = espressif32@6.3.2
    board    = adafruit_feather_esp32_v2
    framework = arduino
    ; Exclude test sub-sketches and duplicate utility copies
    build_src_filter = +<*> -<Tests/> -<SmartFall/> -<Utils/>
    ; Custom partitions sized for 8 MB flash (2 MB app partitions)
    board_build.partitions = partitions.csv

    lib_deps =
        adafruit/Adafruit MPU6050@^2.2.4
        adafruit/Adafruit Unified Sensor@^1.1.9
        adafruit/Adafruit BMP280 Library@^2.6.8
        sparkfun/SparkFun MAX3010x Pulse and Proximity Sensor Library@^1.1.2
        dfrobot/DFRobot_BloodOxygen_S@^1.0.0
        dfrobot/DFRobot_RTU@^1.0.6
        bblanchon/ArduinoJson@^6.21.3
    ```

    ### 4. Build & Upload

    - **Build**: Ctrl+Alt+B (or PlatformIO: Build)
    - **Upload**: Ctrl+Alt+U (or PlatformIO: Upload)
    - **Monitor**: Ctrl+Alt+J (or PlatformIO: Monitor)

=== "Arduino CLI (Command Line)"

    Already covered in [Quick Start](quick-start.md)

## Library Installation Details

### Library Versions Summary

| Library | Version | Purpose |
|---------|---------|---------|
| Adafruit MPU6050 | 2.2.4+ | 6-axis IMU sensor driver |
| Adafruit Unified Sensor | 1.1.9+ | Sensor abstraction layer |
| Adafruit BMP280 Library | 2.6.8+ | Pressure & temperature sensor |
| SparkFun MAX3010x | 1.1.2+ | Optical heart rate sensor |
| DFRobot_BloodOxygen_S | 1.0.0+ | MAX30102 enhanced driver |
| DFRobot_RTU | 1.0.6+ | Modbus library for MAX30102 |
| ArduinoJson | 6.21.3+ | JSON encoding/decoding |

### Installation Troubleshooting

**Library not found error?**
1. Check spelling exactly as shown above
2. Arduino IDE: Try installing from GitHub URL if not in library manager
3. PlatformIO: Check `platformio.ini` syntax
4. Restart IDE after installing libraries

## Port Configuration

### Finding Your Serial Port

=== "Linux/macOS"

    ```bash
    arduino-cli board list
    # Look for entries like:
    # /dev/ttyUSB0   - ESP32 HUZZAH32
    # /dev/ttyACM0   - Alternative USB driver
    ```

=== "Windows"

    ```powershell
    arduino-cli board list
    # Look for entries like:
    # COM3           - Your device
    # Check Device Manager if unsure
    ```

### Setting Default Port

**Arduino CLI** - Edit `SmartFall/sketch.yaml`:
```yaml
default_port: /dev/ttyUSB0
```

**Arduino IDE** - Tools → Port → Select from dropdown

**PlatformIO** - Set in `platformio.ini` (auto-detected usually)

## Verification

### Check ESP32 Installation

```bash
# Arduino CLI
arduino-cli core list

# Output should include:
# esp32:esp32  2.0.x [installed]
```

### Check Libraries

```bash
# Arduino CLI
arduino-cli lib list

# Should include all required libraries above
```

### Compile Test Sketch

```bash
cd SmartFall
arduino-cli compile --fqbn esp32:esp32:adafruit_feather_esp32_v2 .
```

If compilation succeeds, you're ready to upload!

## USB Driver Setup (if needed)

### CP2104 Driver (Adafruit Feather V2)

- **Windows**: Download from [Silicon Labs](https://www.silabs.com/developers/usb-to-uart-bridge-vcp-drivers)
- **Linux**: Usually auto-detected, no driver needed
- **macOS**: Download from Silicon Labs or use `brew`

### CH340 Driver (some clones)

- **Windows/Mac**: Download from [WCH Electronics](http://www.wch-ic.com/downloads/CH340SER_ZIP.html)
- **Linux**: Auto-detected with newer kernels

## Next Steps

1. **Quick Test**: [Quick Start Guide](quick-start.md)
2. **Test Components**: See [Component Tests](../testing/component-tests.md)
3. **Configure System**: Edit [Config.h](../configuration/config-reference.md)
4. **Troubleshoot**: See [Troubleshooting Guide](../troubleshooting.md)

## Advanced: Custom Board Configuration

To use a different ESP32 variant:

1. Edit `platformio.ini` and add new environment:
   ```ini
   [env:custom_esp32]
   platform = espressif32
   board = esp32dev
   framework = arduino
   lib_deps = ... (same as feather_esp32_v2)
   ```

2. Or in Arduino IDE: Tools → Board → Select new board

3. Adjust pin definitions in `SmartFall/Config.h` if GPIO pins differ
