# SmartFall Hardware Documentation

Welcome to the **SmartFall Hardware** documentation. SmartFall is a comprehensive IoT wearable fall detection system using the ESP32 Feather microcontroller with multi-sensor data fusion and emergency alert capabilities.

## 🎯 Project Overview

SmartFall provides real-time fall detection using a sophisticated 5-stage algorithm that analyzes data from multiple sensors:

- **6-Axis IMU (MPU6050)**: Acceleration and gyroscope data
- **Pressure Sensor (BMP280)**: Altitude change detection
- **Heart Rate Monitor (MAX30102)**: Physiological stress validation
- **Force Sensor (FSR)**: Impact and attachment detection

The system triggers multi-modal alerts (audio, haptic, visual) and sends emergency notifications via WiFi and Bluetooth.

## ✨ Key Features

=== "Detection"

    - **5-stage algorithm** with confidence scoring
    - **Multi-sensor fusion** for accurate detection
    - **SOS button** for manual emergency override
    - **Smart filtering** to minimize false positives

=== "Communication"

    - **WiFi** integration for cloud alerts
    - **Bluetooth Low Energy** for mobile app integration
    - **Redundant transmission** for reliability
    - **Auto-reconnect** on network failure

=== "Hardware"

    - **ESP32 Feather V2** with 8MB Flash, 2MB PSRAM
    - **PAM8302 Audio Amplifier** for alerts
    - **4000-5000 mAh** LiPo battery
    - **40-50 hours** estimated battery life

=== "Audio System"

    - **Pre-defined alert patterns** (beeps, sirens, SOS)
    - **Voice-like alerts** for user guidance
    - **Volume control** (0-100%)
    - **Low power consumption** at idle

## 🚀 Quick Links

<div class="grid cards" markdown>

- **[Quick Start](getting-started/quick-start.md)**

  Set up and upload firmware in 5 minutes using Arduino CLI or PlatformIO

- **[Hardware Overview](hardware/overview.md)**

  Components, board comparison, and wiring specifications

- **[Algorithm Guide](algorithm/overview.md)**

  Understand the 5-stage fall detection pipeline

- **[API Reference](api/wifi-endpoints.md)**

  WiFi REST endpoints and BLE protocol specification

- **[Component Testing](testing/component-tests.md)**

  Test individual sensors and modules

- **[Troubleshooting](troubleshooting.md)**

  10 common issues and solutions

</div>

## 📊 System Architecture

```
┌─────────────────────────────────────────────────────┐
│          ESP32 Feather V2 (Main Controller)         │
│                                                     │
│  ┌──────────────────────────────────────────────┐  │
│  │            Sensor Data Acquisition            │  │
│  │  • MPU6050 (Accel + Gyro)  @100Hz            │  │
│  │  • BMP280 (Pressure)                          │  │
│  │  • MAX30102 (Heart Rate)                      │  │
│  │  • FSR (Impact)                               │  │
│  └──────────────────────────────────────────────┘  │
│                        ↓                            │
│  ┌──────────────────────────────────────────────┐  │
│  │      5-Stage Fall Detection Algorithm        │  │
│  │  1. Free Fall Detection                       │  │
│  │  2. Impact Analysis                           │  │
│  │  3. Rotation Assessment                       │  │
│  │  4. Inactivity Check                          │  │
│  │  5. False Positive Filters                    │  │
│  └──────────────────────────────────────────────┘  │
│                        ↓                            │
│  ┌──────────────────────────────────────────────┐  │
│  │       Confidence Scoring & Decision           │  │
│  │  Threshold: 76+ = HIGH confidence alert       │  │
│  └──────────────────────────────────────────────┘  │
│                        ↓                            │
│  ┌──────────────────────────────────────────────┐  │
│  │         Multi-Modal Alert System              │  │
│  │  • Audio (PAM8302 + Speaker)                 │  │
│  │  • Haptic (Vibration motor - optional)       │  │
│  │  • Visual (LED indicator)                     │  │
│  │  • 30-second user response window             │  │
│  └──────────────────────────────────────────────┘  │
│                        ↓                            │
│  ┌──────────────────────────────────────────────┐  │
│  │      Emergency Communication                  │  │
│  │  ├─ WiFi → HTTP POST to web server           │  │
│  │  └─ BLE → Mobile app notification            │  │
│  └──────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────┘
```

## 🔧 Hardware Requirements

- **Microcontroller**: ESP32 Feather V2 (or HUZZAH32)
- **Sensors**: MPU6050, BMP280, MAX30102, FSR
- **Audio**: PAM8302 amplifier + 8Ω speaker
- **Power**: 3.7V LiPo battery (4000-5000 mAh)
- **Optional**: Haptic motor, visual LED, push button

## 📚 Documentation Structure

- **[Getting Started](getting-started/)** - Setup and upload guides
- **[Hardware](hardware/)** - Components and wiring diagrams
- **[Firmware](firmware/)** - Code architecture and module APIs
- **[Algorithm](algorithm/)** - Detailed detection logic
- **[Configuration](configuration/)** - All settings reference
- **[API Reference](api/)** - WiFi and BLE specifications
- **[Testing](testing/)** - Component and system tests
- **[Troubleshooting](troubleshooting.md)** - Common issues

## 🤝 Support

For detailed information:

1. Check the relevant section in this documentation
2. Review the [Troubleshooting](troubleshooting.md) guide
3. Test individual components using [Component Tests](testing/component-tests.md)
4. Open an issue on [GitHub](https://github.com/Smart-Fall/Hardware)

## 📄 License

MIT License - See repository for details

---

**Ready to get started?** → [Quick Start Guide](getting-started/quick-start.md)
