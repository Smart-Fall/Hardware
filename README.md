# SmartFall — Wearable Fall Detection System

Real-time fall detection using multi-sensor data fusion on ESP32, with WiFi/BLE emergency alerts and audio feedback.

**Full Documentation:** [smart-fall.github.io/Hardware](https://smart-fall.github.io/Hardware)

## Quick Start

```bash
# Install ESP32 core and libraries
arduino-cli core install esp32:esp32
arduino-cli lib install "Adafruit MPU6050" "Adafruit BMP280 Library" \
  "Adafruit Unified Sensor" "DFRobot_BloodOxygen_S" "DFRobot_RTU" "ArduinoJson"

# Compile and upload (ESP32 Feather V2)
cd SmartFall
arduino-cli compile --fqbn esp32:esp32:adafruit_feather_esp32_v2 .
arduino-cli upload -p PORT --fqbn esp32:esp32:adafruit_feather_esp32_v2 .
arduino-cli monitor -p PORT -c baudrate=115200
```

> Edit `SmartFall/sketch.yaml` to set your default COM port. For the original HUZZAH32 Feather, use `esp32:esp32:featheresp32` instead.

## Hardware

| Component | Purpose |
|-----------|---------|
| ESP32 Feather V2 | Microcontroller (WiFi + BLE) |
| MPU6050 | 6-axis IMU (accelerometer + gyroscope) |
| BMP280 | Barometric pressure / altitude |
| MAX30102 | Heart rate + SpO2 |
| FSR | Force-sensitive resistor (impact) |
| PAM8302 | 2.5W audio amplifier |

All I2C sensors share the same bus (SDA/SCL). See the [wiring guide](https://smart-fall.github.io/Hardware/hardware/wiring/) for pin assignments.

## How It Works

A 5-stage algorithm scores fall confidence (0–100):

1. **Free fall** — acceleration drops below 0.5g
2. **Impact** — spike above 3.0g
3. **Rotation** — angular velocity exceeds 250 dps
4. **Inactivity** — no movement for 2+ seconds
5. **Validation** — pressure change, FSR, heart rate

Scores above 76 trigger an immediate emergency alert via WiFi (HTTP POST) with BLE as fallback.

## Configuration

All settings are in [`SmartFall/Config.h`](SmartFall/Config.h) — WiFi credentials, server URL, detection thresholds, power management, and debug flags.

## Testing

| Command | Action |
|---------|--------|
| `T` (serial) | Trigger manual fall test |
| `S` (serial) | Print current sensor readings |
| SOS button | Manual emergency alert |

Individual component tests are in `SmartFall/tests/`.

## Documentation

| Topic | Link |
|-------|------|
| Getting Started | [Quick Start](https://smart-fall.github.io/Hardware/getting-started/quick-start/) |
| Hardware & Wiring | [Hardware Overview](https://smart-fall.github.io/Hardware/hardware/overview/) |
| Firmware Architecture | [Architecture](https://smart-fall.github.io/Hardware/firmware/architecture/) |
| Fall Detection Algorithm | [Detection Stages](https://smart-fall.github.io/Hardware/algorithm/stages/) |
| WiFi & BLE APIs | [API Reference](https://smart-fall.github.io/Hardware/api/wifi-endpoints/) |
| Power Management | [Power Budget](https://smart-fall.github.io/Hardware/hardware/power/) |
| Troubleshooting | [Common Issues](https://smart-fall.github.io/Hardware/troubleshooting/) |

## License

MIT License - Feel free to use this project for educational and research purposes.
