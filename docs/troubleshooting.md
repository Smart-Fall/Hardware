# Troubleshooting Guide

Solutions to 10 common SmartFall issues and how to diagnose them.

## 1. Sensor Initialization Fails

**Error**: "ERROR: Failed to initialize [SENSOR]!"

### Root Causes

| Cause | Symptoms | Solution |
|-------|----------|----------|
| **I2C wiring loose** | All I2C sensors fail | Check GPIO 20/22 connections |
| **Wrong I2C address** | Only one sensor fails | Verify address with I2C scanner |
| **Power supply issues** | Device brown-out resets | Check 3.3V voltage (should be 3.3V ±0.1V) |
| **Defective sensor** | Doesn't respond to I2C | Swap with known good sensor |

### Diagnostic Steps

**Step 1: Check Wiring**
```bash
# All I2C sensors use GPIO 20 (SCL) and GPIO 22 (SDA)
# Verify connections with multimeter:
Multimeter Continuity Test:
├─ GPIO 20 → MPU6050 SCL ✓
├─ GPIO 20 → BMP280 SCL ✓
├─ GPIO 20 → MAX30102 SCL ✓
├─ GPIO 22 → MPU6050 SDA ✓
├─ GPIO 22 → BMP280 SDA ✓
└─ GPIO 22 → MAX30102 SDA ✓
```

**Step 2: I2C Scanner Test**

Upload the I2C scanner sketch:
```cpp
#include <Wire.h>

void setup() {
  Serial.begin(115200);
  Wire.begin(22, 20);  // SDA=22, SCL=20
  Serial.println("Scanning I2C bus...");
}

void loop() {
  for (uint8_t addr = 1; addr < 127; addr++) {
    Wire.beginTransmission(addr);
    if (Wire.endTransmission() == 0) {
      Serial.print("Found device at 0x");
      Serial.println(addr, HEX);
    }
  }
  delay(5000);
}
```

Expected output:
```
Found device at 0x68  (MPU6050)
Found device at 0x76  (BMP280)
Found device at 0x57  (MAX30102)
```

**Step 3: Voltage Check**
```
3.3V rail: Should be 3.25-3.35V
3.3V drop per sensor: <0.05V acceptable
Total I2C load: <10mA normal
```

### Solutions

**I2C Pull-ups Missing?**
```
ESP32 has internal 100kΩ pull-ups on GPIO 20/22
External pull-ups rarely needed unless:
├─ Very long wires (>0.5m)
├─ Many devices on bus
└─ Using old I2C sensors with high impedance
```

**MAX30102 Won't Initialize?**
```cpp
// Check reset pin (GPIO 21)
// Reset is active LOW - may be pulled down
#define MAX30102_RST_PIN A9  // GPIO 21

// If reset failing:
// 1. Check GPIO 21 physically pulled LOW
// 2. Try hardware reset:
pinMode(21, OUTPUT);
digitalWrite(21, LOW);
delay(10);
digitalWrite(21, HIGH);
delay(100);
```

**Power Supply Sagging?**
```
Symptoms:
├─ Works briefly then crashes
├─ Brown-out resets
└─ Sensors intermittent

Solution:
├─ Add 100µF capacitor to 3.3V rail
├─ Check battery voltage > 3.5V
└─ Reduce other loads
```

## 2. Upload Fails / Port Not Found

**Error**: "Board not found" or "Permission denied"

### Root Causes

| Cause | Solution |
|-------|----------|
| **Wrong USB cable** | Use data cable, not charge-only |
| **Port not detected** | Install CP2104 or CH340 driver |
| **Wrong board selected** | Check Tools → Board setting |
| **Permissions (Linux)** | Add user to dialout group |

### Solutions

**Windows: Install USB Drivers**

1. Download CP2104 driver: https://www.silabs.com/developers/usb-to-uart-bridge-vcp-drivers
2. Extract and run installer
3. Restart computer
4. Board should appear in port list

**Linux: Fix Permissions**
```bash
# Add user to dialout group
sudo usermod -a -G dialout $USER

# Apply immediately
newgrp dialout

# Restart Arduino IDE
```

**macOS: Trust Drivers**
```bash
# Allow driver in System Preferences if prompted
# Or install with Homebrew:
brew install wch-ch34x-usb-serial-driver
```

**Test Connection**
```bash
# List available ports
arduino-cli board list

# Should show:
# /dev/ttyUSB0     (or COM3, /dev/cu.* on Mac)
# "ESP32 Feather" board
```

## 3. No Serial Output

**Symptom**: Serial monitor shows nothing

### Root Causes

| Cause | Fix |
|-------|-----|
| **Wrong baud rate** | Must be 115200 |
| **No data being sent** | Check Serial.begin() in sketch |
| **USB cable not data** | Replace with data cable |
| **Port permission denied** | See #2 above |

### Solutions

**Verify Baud Rate**
```
Arduino IDE → Tools → Serial Monitor → 115200 baud
Arduino CLI: monitor -p PORT -c baudrate=115200
```

**Check Serial.begin()**
```cpp
// Must match:
Serial.begin(115200);
// And in monitor settings: 115200 baud
```

**Test with Simple Sketch**
```cpp
void setup() {
  Serial.begin(115200);
  Serial.println("Hello!");
}

void loop() {
  delay(1000);
}
```

If still no output, power issue likely.

## 4. WiFi Connection Fails

**Error**: "WiFi connection failed" or "Cannot reach server"

### Root Causes

| Cause | Solution |
|-------|----------|
| **Wrong SSID/password** | Verify in Config.h |
| **2.5GHz network** | ESP32 only supports 2.4GHz |
| **WPA3 encryption** | Not fully supported; use WPA2 |
| **Server unreachable** | Check URL and network routing |

### Solutions

**Verify Credentials**
```cpp
// In Config.h, check:
#define WIFI_SSID      "YourNetworkName"  // Exact match, case-sensitive
#define WIFI_PASSWORD  "YourPassword"     // Must be exact
```

**Check Network Type**
```
Router settings:
├─ 2.4GHz: ✓ Works
├─ 5GHz: ✗ Not supported
└─ Dual-band: Use 2.4GHz band
```

**Test WiFi Manually**
```bash
# From computer on same network
ping -c 4 192.168.1.100  # Or your router IP

# Check what 2.4GHz networks are available
iwlist wlan0 scan  (Linux)
netsh wlan show networks (Windows)
```

**Debug WiFi Connection**
```cpp
#define DEBUG_COMMUNICATION true  // In Config.h

// Serial output will show:
// Scanning for WiFi...
// Found network: "YourNetwork"
// Attempting to connect...
// Connected! IP: 192.168.1.100
// WiFi signal: -45 dBm
```

**Server Unreachable?**
```cpp
#define SERVER_URL "http://10.129.112.75:3000"
// Try:
// 1. Check URL is correct
// 2. Verify server is running
// 3. Check firewall rules
// 4. Test from command line:
curl http://10.129.112.75:3000/api/emergency
```

## 5. BLE Not Discoverable

**Symptom**: "SmartFall" doesn't appear in BLE scanner

### Root Causes

| Cause | Solution |
|-------|----------|
| **BLE not initialized** | Check serial output |
| **Phone too far away** | Move within 2 meters |
| **BLE disabled** | Enable Bluetooth on phone |
| **Device already connected** | Disconnect first |

### Solutions

**Verify BLE is Initialized**
```cpp
// Serial output should show:
// ✓ BLE server started
// ✓ Device name: SmartFall
// ✓ Service UUID created
// ✓ Advertising started...
```

**Check Device Range**
- BLE range typically 5-10 meters (indoor)
- Metal objects interfere
- Move to open area for testing

**Test with BLE Scanner App**
```
iOS:    LightBlue, BLE Scanner
Android: nRF Connect, BLE Scanner

Steps:
1. Open app
2. Scan for devices
3. Look for "SmartFall"
4. Click to connect
5. View characteristics
```

**Power Cycling**
```cpp
// Restart device if BLE stuck
Power cycle: Disconnect battery, wait 5 seconds, reconnect
```

## 6. Fall Detection Not Triggering

**Symptom**: Drop device but no alert occurs

### Root Causes

| Cause | Solution |
|-------|----------|
| **Low confidence threshold** | Algorithm correctly avoiding false positive |
| **Sensors not initialized** | Check initialization output |
| **Thresholds too strict** | Lower FREEFALL_THRESHOLD_G in Config.h |
| **Detection disabled** | Check enableMonitoring() called |

### Solutions

**Enable Debug Output**
```cpp
#define DEBUG_ALGORITHM_STEPS true  // In Config.h

// Shows:
// Stage 1: Free fall detected (duration: XXXms, accel: XXXg) → +Xpts
// Stage 2: Impact detected...
// etc.
```

**Lower Thresholds**
```cpp
// In Config.h, try:
#define FREEFALL_THRESHOLD_G    0.6f   // Was 0.5f (more sensitive)
#define IMPACT_THRESHOLD_G      2.5f   // Was 3.0f (catches softer landings)
#define ROTATION_THRESHOLD_DPS  120.0f // Was 150.0f (detects slower rotation)
```

**Test with SOS Button**
```
GPIO 15 SOS button should:
├─ Trigger immediately (no delay)
├─ Play loud siren
└─ Send emergency alert

If SOS works but motion doesn't → Algorithm issue
If both fail → Communication issue
```

**Verify Sensor Data**
```cpp
#define DEBUG_SENSOR_DATA true  // Show raw readings

Watch for:
├─ Free fall phase: acceleration < 0.5g
├─ Impact phase: acceleration > 3.0g
├─ Inactivity: stable 0.8-1.2g for 2+ seconds
```

## 7. False Positive Alerts

**Symptom**: Alerts trigger during normal activity (exercise, playing, etc.)

### Root Causes

| Cause | Solution |
|-------|----------|
| **Thresholds too low** | Raise FREEFALL_THRESHOLD_G |
| **Confidence thresholds too low** | Raise HIGH_CONFIDENCE_THRESHOLD |
| **Noisy sensor data** | Check sensor calibration |

### Solutions

**Raise Detection Thresholds**
```cpp
#define FREEFALL_THRESHOLD_G       0.4f   // Higher = less sensitive
#define ROTATION_THRESHOLD_DPS     200.0f // Higher = ignore fast motion
#define HIGH_CONFIDENCE_THRESHOLD  80     // Higher = require more evidence
```

**Analyze False Positive**
```cpp
// Before raising thresholds, understand what triggered:
// Serial output shows:
// Stage 1: Free fall detected (duration: XXms, accel: XXg) → +Xpts
// Stage 2: Impact detected (accel: XXg) → +Xpts
// ...
// TOTAL: XX pts

// If total is just below threshold:
// ├─ Legitimate near-miss
// └─ Raise thresholds to prevent

// If total is well above threshold:
// ├─ Normal activity generating high scores
// └─ Reduce sensor sensitivity or use filters
```

**Context-Aware Filtering**
```cpp
// Experimental: Disable alerts during known activity windows
#define QUIET_HOURS_START  22  // 10 PM
#define QUIET_HOURS_END    6   // 6 AM

// During exercise hours: Don't alert unless SOS pressed
// During sleep hours: Lower threshold slightly (more alert)
```

## 8. Low Battery / Power Issues

**Symptom**: "Low battery warning" or device shuts down

### Root Causes

| Cause | Solution |
|-------|----------|
| **Battery depleted** | Charge with USB-C cable |
| **High power load** | Disable WiFi/audio during low battery |
| **WiFi draining battery** | Reduce WiFi transmission frequency |
| **Defective battery** | Test with multimeter, replace if needed |

### Solutions

**Check Battery Voltage**
```
Normal range: 3.5V - 4.2V (3.7V LiPo nominal)
Low warning: < 3.3V
Critical: < 3.0V → Device won't function

Test with multimeter between JST+ and JST-
```

**Monitor Battery in Serial**
```cpp
#define DEBUG_SENSOR_DATA true

// Shows:
// Battery: 85% (3.85V)
// Battery: 10% (3.2V) ← Low warning triggered
// Battery: 2% (3.05V) ← Critical, may shut down
```

**Power Optimization**
```cpp
// In Config.h:
#define WIFI_RECONNECT_INTERVAL_MS 60000  // Try WiFi less often
#define AUDIO_DEFAULT_VOLUME       50     // Lower volume
#define BLE_STREAMING_INTERVAL_MS  2000   // Less frequent updates

// Or use deep sleep:
#define ENABLE_DEEP_SLEEP true  // Experimental
```

**Charge Battery**
```
USB-C cable:
  - Feather V2: USB-C port on side
  - HUZZAH32: Micro-USB port

Charging status:
  - LED indicates charging (check Adafruit docs)
  - ~8-10 hours for full charge (4000mAh)
  - Full = 4.2V, Empty = 3.0V
```

## 9. No Audio Output

**Symptom**: Audio system silent (no beeps or alerts)

### Root Causes

| Cause | Solution |
|-------|----------|
| **Speaker not connected** | Check PAM8302 OUT+ / OUT- |
| **PAM8302 not powered** | Verify 3.3V and GND |
| **Volume set to 0** | Check AUDIO_DEFAULT_VOLUME |
| **GPIO 25 PWM issue** | Verify pin not used for something else |

### Solutions

**Test Speaker Alone**
```
1. Disconnect PAM8302
2. Test speaker with battery:
   ├─ Speaker+ to 3.3V
   └─ Speaker- to GND
3. Should click
```

**Check PAM8302 Wiring**
```
ESP32        PAM8302      Speaker
GPIO 25(PWM) → A+
GND ────────→ A-
              └─ GND ────→ Speaker (-)
3.3V ────────→ VDD
              └─ OUT+ ────→ Speaker (+)
              └─ OUT- ────→ Speaker (-) / GND
```

**Verify GPIO 25 Free**
```cpp
// Check Config.h - GPIO 25 shouldn't be used elsewhere:
#define SPEAKER_PIN A12  // GPIO 25 (correct)

// Not:
#define SOME_OTHER_PIN  25  // Would conflict!
```

**Test with Audio Test Sketch**
```bash
cd SmartFall/tests/Audio
arduino-cli upload -p PORT --fqbn $BOARD_FQBN .
# Listen for audio patterns
```

**Add Power Filtering**
```
If audio present but distorted:
├─ Add 100µF capacitor between VDD and GND
├─ Add 10µF capacitor on audio input (PAM A+)
└─ Use separate battery supply for PAM8302
```

## 10. Device Crashes / Resets

**Symptom**: Unexpected resets, error messages, or locked up behavior

### Root Causes

| Cause | Solution |
|-------|----------|
| **Insufficient power** | Check 3.3V supply and capacitors |
| **Stack overflow** | Reduce history buffer size |
| **Watchdog timeout** | Check main loop delays |
| **Memory leak** | Look for unfreed allocations |

### Solutions

**Monitor Reset Reason**
```cpp
// Add to setup():
esp_reset_reason_t reason = esp_reset_reason();
switch(reason) {
    case ESP_RST_POWERON:
        Serial.println("Power on");
        break;
    case ESP_RST_EXT:
        Serial.println("External reset");
        break;
    case ESP_RST_WDT:
        Serial.println("Watchdog timeout ← Check main loop");
        break;
    case ESP_RST_BROWNOUT:
        Serial.println("Brown-out ← Check power supply");
        break;
    default:
        Serial.println("Other reset reason");
}
```

**Check Power Supply**
```
With multimeter:
├─ VCC should stay above 3.25V under load
├─ If drops below 3.0V → Brown-out reset
└─ Add larger capacitor if dropping more than 0.2V
```

**Watch for Loop Delays**
```cpp
// Main loop must complete in <10ms
unsigned long start = millis();
// ... processing ...
unsigned long elapsed = millis() - start;
if (elapsed > 15) {
    Serial.print("Slow loop: ");
    Serial.print(elapsed);
    Serial.println("ms ← Watchdog may timeout");
}
```

**Check Memory Usage**
```cpp
// Add to loop:
Serial.print("Free heap: ");
Serial.print(ESP.getFreeHeap());
Serial.println(" bytes");

// Should stay > 100KB
// If drops below 50KB, memory leak likely
```

## General Debugging Tips

### Enable Comprehensive Debugging
```cpp
#define DEBUG_SENSOR_DATA       true
#define DEBUG_ALGORITHM_STEPS   true
#define DEBUG_COMMUNICATION     true
#define DEBUG_AUDIO             true
```

### Create Log File
```bash
# Capture serial output to file:
arduino-cli monitor -p PORT > smartfall.log &

# Analyze later:
grep "ERROR" smartfall.log
grep "Stage 1" smartfall.log
```

### Test Individual Components
Always test components in isolation before integrating:
```
1. Sensors (MPU6050, BMP280, MAX30102, FSR)
2. Audio (PAM8302 + speaker)
3. WiFi connectivity
4. BLE advertisement
5. Fall detection algorithm
6. Full system integration
```

## When to Seek Help

Create detailed bug report including:
- [ ] Serial output showing error
- [ ] What you were trying to do
- [ ] What happened instead
- [ ] Device model (Feather V2 or HUZZAH32)
- [ ] SmartFall code version
- [ ] Wiring diagram or photo

Post on:
- GitHub Issues: https://github.com/Smart-Fall/Hardware/issues
- Arduino Forum: https://forum.arduino.cc/
- Adafruit Support: https://learn.adafruit.com/

---

**Remember**: Most issues are resolved with careful wiring verification, driver installation, and checking the serial debug output!
