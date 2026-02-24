# Hardware Overview

Complete hardware specifications for the SmartFall wearable fall detection system.

## Microcontroller (Choose One)

### ESP32 Feather V2 (Recommended)

- **Manufacturer**: Adafruit
- **Model**: #5400
- **Processor**: Tensilica LX6 dual-core 240MHz
- **Flash Memory**: 8MB (double the original)
- **PSRAM**: 2MB (new addition)
- **Connectivity**: WiFi 802.11 b/g/n, Bluetooth 5.0 LE
- **Sleep Current**: 70µA deep sleep (ultra-low power)
- **Features**:
  - USB Type-C (modern connector)
  - Built-in NeoPixel RGB LED
  - STEMMA QT connector for easy I2C
  - Enhanced battery charging
- **Product Page**: [Adafruit #5400](https://www.adafruit.com/product/5400)

### ESP32 HUZZAH32 Feather (Original)

- **Manufacturer**: Adafruit
- **Model**: #3405
- **Processor**: Tensilica LX6 dual-core 240MHz
- **Flash Memory**: 4MB
- **PSRAM**: None
- **Connectivity**: WiFi 802.11 b/g/n, Bluetooth 5.0 LE
- **Features**:
  - Micro USB
  - Breadboard-friendly Feather form factor
  - JST battery connector
- **Product Page**: [Adafruit #3405](https://www.adafruit.com/product/3405)

**Note**: Both boards are fully compatible with SmartFall. Choose Feather V2 for enhanced performance and low power consumption.

## Sensor Components

| Component | Model | Function | Interface | Address/Pin |
|-----------|-------|----------|-----------|------------|
| **Accelerometer/Gyroscope** | MPU6050 | 6-axis IMU for motion detection | I2C | 0x68 |
| **Pressure Sensor** | BMP280 | Altitude change detection | I2C | 0x76/0x77 |
| **Heart Rate Monitor** | MAX30102 | Heart rate & SpO2 optical sensor | I2C | 0x57 |
| **Force Sensor** | FSR (Analog) | Impact and strap tension | Analog | GPIO 34 |

## Audio System

| Component | Model | Specification | Purpose |
|-----------|-------|---------------|---------|
| **Amplifier** | PAM8302 | 2.5W Class D mono | Amplifies audio from ESP32 |
| **Speaker** | Generic 8Ω | 0.5W - 1W rated | Delivers alert audio |
| **PWM Output** | GPIO 25 | 5 kHz, 8-bit | Audio signal from ESP32 |

## Power System

| Component | Specification | Capacity | Connector |
|-----------|---------------|----------|-----------|
| **Battery** | Lithium Polymer (LiPo) | 4000-5000 mAh, 3.7V | JST 2-pin |
| **Charging** | USB-C (V2) / Micro-USB (Original) | 1A input | Onboard charger |
| **Voltage Regulation** | 3.3V linear | 500mA typical | Integrated |

## Optional Components

- **Haptic Motor**: Vibration feedback (GPIO 26)
- **Visual LED**: Status indicator (GPIO 27)
- **OLED Display**: System status (not currently used)

## Pin Assignment Summary

### I2C Sensors (Shared Bus)

```
GPIO 20 (SCL) ──┬── MPU6050 (SCL)
                ├── BMP280 (SCL)
                └── MAX30102 (SCL)

GPIO 22 (SDA) ──┬── MPU6050 (SDA)
                ├── BMP280 (SDA)
                └── MAX30102 (SDA)

GPIO 21 ──────── MAX30102 (RST - Reset)
```

### Analog Inputs (ADC)

| Sensor | GPIO | ADC Bank | Function |
|--------|------|----------|----------|
| FSR | GPIO 34 (A2) | ADC1 | Force/Impact detection (WiFi safe) |
| Battery Monitor | GPIO 36 (A4) | ADC1 | Battery voltage monitoring (WiFi safe) |

### Digital I/O

| Component | GPIO | Type | Function |
|-----------|------|------|----------|
| SOS Button | GPIO 15 | INPUT_PULLUP | Emergency override |
| Speaker | GPIO 25 | PWM Output | Audio amplifier input |
| Haptic Motor | GPIO 26 | Digital Output | Vibration feedback |
| Visual LED | GPIO 27 | Digital Output | Status indicator |

## Power Consumption

| Component | Active Current | Idle/Sleep |
|-----------|----------------|-----------|
| **ESP32 (WiFi/BLE)** | ~80 mA | 70 µA (deep sleep) |
| **MPU6050** | ~3.9 mA | 0.8 mA |
| **BMP280** | ~0.7 mA | 0.1 mA |
| **MAX30102** | ~10 mA | 2 mA |
| **FSR Sensors** | ~0.6 mA | ~0.6 mA |
| **PAM8302 (silent)** | ~50 mA | 50 mA |
| **PAM8302 (audio)** | 200-300 mA | - |
| **Haptic Motor** | 50-100 mA | 0 mA |
| **Visual LED** | ~20 mA | 0 mA |

### Typical Operation

- **Normal Operation**: ~95 mA average
- **Peak (alerts active)**: 350-400 mA
- **Estimated Battery Life**: 40-50 hours at average current

## Board Comparison

| Feature | Feather V2 | HUZZAH32 |
|---------|-----------|----------|
| **Flash** | 8MB | 4MB |
| **PSRAM** | 2MB | None |
| **Connector** | USB-C | Micro-USB |
| **Sleep Current** | 70µA | Higher |
| **Price** | ~$25 | ~$20 |
| **Recommended** | ✓ Yes | Compatible |

## Wiring Checklist

See [Wiring Guide](wiring.md) for complete connection details, but key points:

- [ ] All I2C sensors on shared GPIO 20/22 bus
- [ ] MAX30102 reset pin connected to GPIO 21
- [ ] FSR and battery monitor on ADC1 pins (WiFi safe)
- [ ] PAM8302 receives PWM from GPIO 25
- [ ] SOS button on GPIO 15 with internal pull-up
- [ ] All 3.3V and GND connections secure

## Next Steps

1. **Wiring**: See [Wiring Guide](wiring.md)
2. **Power Budget**: See [Power Calculations](power.md)
3. **Sensor Details**: See [Firmware Sensors](../firmware/sensors.md)
4. **Component Tests**: See [Testing Guide](../testing/component-tests.md)
