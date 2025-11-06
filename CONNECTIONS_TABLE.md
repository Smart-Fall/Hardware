# SmartFall Hardware Connections Table

## ESP32 Feather V2 Pin Assignments

### Power Distribution
| Component | Pin | Voltage | Description |
|-----------|-----|---------|-------------|
| 3.3V Rail | 3V | +3.3V | Main power supply from battery regulator |
| Ground Rail | GND | 0V | Ground return for all components |
| Battery Monitor | A4 (GPIO 36) | 0-3.3V | Voltage divider input for battery level (ADC1 - WiFi safe) |

---

## I2C Bus Sensors (Shared Bus - GPIO 20 & 22)

### I2C Configuration
| Parameter | Value |
|-----------|-------|
| **I2C Clock (SCL)** | GPIO 20 |
| **I2C Data (SDA)** | GPIO 22 |
| **Speed** | 100 kHz (standard mode) |
| **Voltage** | 3.3V |

### Connected Sensors
| Sensor | I2C Address | Function | Connection |
|--------|-------------|----------|------------|
| **MPU6050** | 0x68 | 6-Axis IMU (Accel + Gyro) | GPIO 20 (SCL), GPIO 22 (SDA), 3V, GND |
| **BMP280** | 0x76 or 0x77 | Pressure + Temperature | GPIO 20 (SCL), GPIO 22 (SDA), 3V, GND |
| **MAX30102** | 0x57 | Heart Rate + SpO2 (Optical) | GPIO 20 (SCL), GPIO 22 (SDA), GPIO 21/MI (RST), 3V, GND |

---

## Analog Sensors (ADC Inputs)

| Sensor | Pin | ADC Bank | ADC Resolution | Range | Function |
|--------|-----|----------|-----------------|-------|----------|
| **FSR #1** | A2 (GPIO 34) | ADC1 | 12-bit | 0-4095 | Force/Impact Detection (WiFi safe) |
| **Battery Sense** | A4 (GPIO 36) | ADC1 | 12-bit | 0-3.3V | Battery Voltage Monitoring (Voltage Divider, WiFi safe) |

### FSR Conditioning Circuits
Each FSR has:
- 22kΩ pull-up resistor to 3.3V
- 0.1µF capacitor to GND (noise filtering)

---

## Digital Outputs (GPIO)

| Component | GPIO Pin | Mode | Frequency/PWM | Function |
|-----------|----------|------|---|----------|
| **Speaker (PAM8302)** | GPIO 25 | PWM Output | 5 kHz, 8-bit | Audio amplifier input (PWM signal) |
| **Haptic Motor** | GPIO 26 | Digital Output | - | Vibration feedback (5 seconds on alert) |
| **Visual Alert LED** | GPIO 27 | Digital Output | - | Status indicator light |

---

## Digital Inputs (GPIO)

| Component | GPIO Pin | Mode | Logic | Function |
|-----------|----------|------|-------|----------|
| **SOS Button** | GPIO 15 | INPUT_PULLUP | Active LOW | Manual emergency override button |

---

## Audio System

| Component | Connection | Specs | Purpose |
|-----------|-----------|-------|---------|
| **PAM8302 Amplifier** | GPIO 25 (PWM) | 2.5W Class D | Amplifies audio signal from ESP32 |
| | 3V, GND | 2.5W output | Power and ground |
| **Speaker** | PAM8302 OUT+ / OUT- | 8Ω, 0.5-1W | Fall detection alerts, voice patterns |

---

## Communication Modules (Built-in to ESP32)

| Module | Protocol | Frequency | Function |
|--------|----------|-----------|----------|
| **WiFi** | 802.11 b/g/n | 2.4 GHz | Emergency alert transmission to server |
| **Bluetooth** | BLE 5.0 | 2.4 GHz | Mobile app communication and notifications |

---

## Power System

| Component | Voltage | Capacity | Connector | Function |
|-----------|---------|----------|-----------|----------|
| **LiPo Battery** | 3.7V nominal | 4000-5000 mAh | JST 2-pin | Main power source |
| **USB-C Charging** | 5V input | - | USB-C port | Battery charging port |
| **Battery Monitor** | 0-3.3V | - | Voltage divider | Battery level sensing via A13 |

---

## Power Budget

| Component | Current Draw | Notes |
|-----------|--------------|-------|
| ESP32 (active) | ~80 mA | WiFi/BLE enabled |
| MPU6050 | ~3.9 mA | Continuous operation |
| BMP280 | ~0.7 mA | Continuous operation |
| MAX30102 | ~10 mA | Continuous operation |
| FSR Sensors | ~0.6 mA | All three combined |
| PAM8302 (silent) | ~50 mA | Idle/standby |
| PAM8302 (audio) | 200-300 mA | Peak during alerts |
| Haptic Motor | ~50-100 mA | When vibrating |
| **Total Average** | **~95 mA** | Normal operation |
| **Total Peak** | **~350-400 mA** | Audio + haptic active |

**Battery Life:** 40-50 hours at average current

---

## Complete Connection Summary

### From ESP32 to Each Component

#### **I2C Sensors (3 devices on shared bus)**
```
GPIO 20 (SCL) ──┬──→ MPU6050 (SCL)
                ├──→ BMP280 (SCL)
                └──→ MAX30102 (SCL)

GPIO 22 (SDA) ──┬──→ MPU6050 (SDA)
                ├──→ BMP280 (SDA)
                └──→ MAX30102 (SDA)

GPIO 21 (MI) ───────→ MAX30102 (RST) [Hardware Reset, active LOW]

3V (Power) ──┬──→ MPU6050 (VDD)
             ├──→ BMP280 (VDD)
             └──→ MAX30102 (VDD)

GND ────┬──→ MPU6050 (GND)
        ├──→ BMP280 (GND)
        └──→ MAX30102 (GND)
```

#### **Analog Sensors**
```
A2 (GPIO 34) ──→ FSR #1 signal (ADC1 - WiFi safe)
A4 (GPIO 36) ──→ Battery Sense (ADC1 - WiFi safe)
```

#### **Audio Output**
```
GPIO 25 (PWM) ──→ PAM8302 A+ (audio input)
GND ──────────→ PAM8302 A- (ground input)
3V ───────────→ PAM8302 VDD (power)
GND ──────────→ PAM8302 GND
PAM8302 OUT+ ──→ Speaker (+)
PAM8302 OUT- ──→ Speaker (-) / GND
```

#### **User Interface**
```
GPIO 26 ──→ Haptic Motor + (power)
GND ────→ Haptic Motor - (ground)

GPIO 27 ──→ Visual Alert LED + (power)
GND ────→ Visual Alert LED - (ground)

GPIO 15 ──→ SOS Button (pull-up internally)
GND ────→ SOS Button (other terminal)
```

#### **Power**
```
Battery JST+ ──→ Battery Manager Charging Circuit ──→ ESP32 VBAT
Battery JST- ──→ GND
USB-C ────────→ Battery charging circuit
```

---

## Key Configuration Notes

- **I2C Pull-ups:** The ESP32 has internal pull-ups on GPIO 20/22 (100 kΩ typical)
- **I2C Pins:** Feather V2 uses GPIO 20 (SCL) and GPIO 22 (SDA) - different from HUZZAH32
- **FSR Pull-ups:** External 22 kΩ resistors required to 3.3V
- **FSR Filtering:** 0.1 µF capacitors on each FSR input to GND
- **ADC Banks:** FSR and Battery use ADC1 pins (GPIO 34, 36) which remain operational with WiFi enabled
- **Battery Divider:** Calculate actual voltage from A4 (GPIO 36) reading (Vbat = ADC_reading * (6.6 / 4095))
- **SOS Button:** Active LOW, internal pull-up keeps pin HIGH, LOW when pressed
- **Audio PWM:** 5 kHz frequency, 8-bit resolution for audio synthesis
- **OLED Display:** Removed - not used in this configuration

