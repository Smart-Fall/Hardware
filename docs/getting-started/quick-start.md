# Quick Start Guide

Get SmartFall up and running in 5 minutes with Arduino CLI or PlatformIO.

## Prerequisites

- ESP32 Feather V2 or HUZZAH32 board
- USB-C cable (for Feather V2) or Micro USB (for HUZZAH32)
- Computer with Arduino CLI or PlatformIO installed
- All required libraries (see [Development Setup](development-setup.md))

## Installation (Choose One Method)

=== "Arduino CLI (Recommended)"

    ### 1. Install Arduino CLI

    **Linux/macOS:**
    ```bash
    curl -fsSL https://raw.githubusercontent.com/arduino/arduino-cli/master/install.sh | sh
    export PATH=$PATH:$HOME/bin
    ```

    **Windows PowerShell:**
    ```powershell
    Invoke-WebRequest -Uri "https://downloads.arduino.cc/arduino-cli/arduino-cli_latest_Windows_64bit.zip" -OutFile "$env:TEMP\arduino-cli.zip"
    Expand-Archive -Path "$env:TEMP\arduino-cli.zip" -DestinationPath "C:\arduino-cli"
    [Environment]::SetEnvironmentVariable("Path", $env:Path + ";C:\arduino-cli", "Machine")
    # Restart PowerShell after setting PATH
    ```

    ### 2. Setup ESP32 Board Support

    ```bash
    arduino-cli config init
    arduino-cli core update-index
    arduino-cli core install esp32:esp32
    ```

    ### 3. Install Required Libraries

    ```bash
    arduino-cli lib install "Adafruit MPU6050" "Adafruit BMP280 Library" \
      "SparkFun MAX3010x Pulse and Proximity Sensor Library" \
      "Adafruit Unified Sensor" "DFRobot_BloodOxygen_S" \
      "DFRobot_RTU" "ArduinoJson"
    ```

    ### 4. Configure Port (Optional)

    Edit `SmartFall/sketch.yaml`:
    ```yaml
    default_port: /dev/ttyUSB0  # Linux
    # default_port: COM3          # Windows
    # default_port: /dev/cu.usbserial  # macOS
    ```

=== "PlatformIO"

    ### 1. Install PlatformIO

    ```bash
    pip install -U platformio
    ```

    ### 2. Libraries Auto-Installed

    PlatformIO automatically installs all dependencies from `platformio.ini`

    ### 3. Configure Board (Optional)

    Edit `platformio.ini` and set your default environment:
    ```ini
    default_envs = feather_esp32_v2
    ; or
    ; default_envs = huzzah32
    ```

## Upload Firmware

=== "Arduino CLI - Feather V2"

    ```bash
    cd SmartFall

    # Find your port
    arduino-cli board list

    # Compile
    arduino-cli compile --fqbn esp32:esp32:adafruit_feather_esp32_v2 .

    # Upload (replace PORT with your actual port: /dev/ttyUSB0, COM3, etc.)
    arduino-cli upload -p PORT --fqbn esp32:esp32:adafruit_feather_esp32_v2 .
    ```

=== "Arduino CLI - HUZZAH32"

    ```bash
    cd SmartFall

    # Find your port
    arduino-cli board list

    # Compile
    arduino-cli compile --fqbn esp32:esp32:featheresp32 .

    # Upload (replace PORT with your actual port)
    arduino-cli upload -p PORT --fqbn esp32:esp32:featheresp32 .
    ```

=== "PlatformIO"

    ```bash
    # For Feather V2
    pio run -e feather_esp32_v2 -t upload

    # For HUZZAH32
    pio run -e huzzah32 -t upload
    ```

## Verify Upload

Monitor serial output to confirm initialization:

```bash
# Arduino CLI
arduino-cli monitor -p PORT -c baudrate=115200

# PlatformIO
pio device monitor
```

Expected output:
```
========================================
      SmartFall Detection System
   Complete with Audio & Communication
========================================

--- Initializing Sensors ---
✓ MPU6050 initialized
✓ BMP280 initialized
✓ MAX30102 heart rate sensor initialized
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

## Common Issues

| Problem | Solution |
|---------|----------|
| "Port not found" | Restart board, check USB cable (data vs charge-only) |
| "Board not recognized" | Install CP2104 or CH340 USB driver |
| "Permission denied" (Linux) | `sudo usermod -a -G dialout $USER` then logout/login |
| "Failed to initialize sensor" | Check I2C wiring, verify sensor addresses |

## Next Steps

1. **Test Sensors**: Run component tests in `SmartFall/tests/`
2. **Configure WiFi**: Edit `SmartFall/Config.h` (WIFI_SSID, WIFI_PASSWORD)
3. **Test Fall Detection**: Use SOS button (GPIO 15) or simulate motion
4. **Read Full Documentation**: See [Getting Started](development-setup.md) for detailed setup

## Troubleshooting

See [Full Troubleshooting Guide](../troubleshooting.md) for detailed solutions to common problems.
