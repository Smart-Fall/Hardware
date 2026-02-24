# Component Tests

Comprehensive testing guide for individual SmartFall hardware components.

## Test Sketches Location

```
SmartFall/tests/
├── MPU6050/         # 6-axis IMU test
├── BMP280/          # Pressure sensor test
├── MAX30102/        # Heart rate sensor test
├── FSR/             # Force sensor test
├── WiFi/            # WiFi connectivity test
├── BLE/             # Bluetooth test
└── Audio/           # Audio system test
```

## Common Setup

All test sketches require the same board configuration:

```bash
# For Feather V2
BOARD_FQBN="esp32:esp32:adafruit_feather_esp32_v2"

# For HUZZAH32
BOARD_FQBN="esp32:esp32:featheresp32"
```

## MPU6050 (6-Axis IMU) Test

Tests accelerometer and gyroscope functionality.

### Upload

```bash
cd SmartFall/tests/MPU6050
arduino-cli compile --fqbn $BOARD_FQBN .
arduino-cli upload -p PORT --fqbn $BOARD_FQBN .
arduino-cli monitor -p PORT -c baudrate=115200
```

### Expected Output

```
────────────────────────────────
    MPU6050 Sensor Test
────────────────────────────────

Initializing MPU6050...
✓ MPU6050 initialized successfully

Reading sensor data (10 samples):

Sample 1:
  Accel: X=0.05g  Y=0.02g  Z=9.82g  (normal gravity)
  Gyro:  X=1°/s   Y=2°/s   Z=0°/s   (minimal rotation)

Sample 2:
  Accel: X=0.03g  Y=0.01g  Z=9.84g
  Gyro:  X=0°/s   Y=1°/s   Z=1°/s

... (more samples)

Test Result: ✓ PASS
```

### Verification Checklist

- [ ] Initialization succeeds
- [ ] Readings update continuously
- [ ] Gravity component ≈ 9.8g on Z-axis
- [ ] X/Y acceleration ≈ 0g when still
- [ ] Gyro values near 0 when device stationary
- [ ] Values change when device is moved/rotated

### Troubleshooting

**"Failed to initialize MPU6050"**
- Check I2C wiring (GPIO 20/22)
- Verify MPU6050 address: `0x68`
- Try I2C scanner test

**No data updates**
- Check I2C pull-ups (100kΩ on ESP32)
- Verify SDA/SCL not swapped
- Restart board

## BMP280 (Pressure & Temperature) Test

Tests barometric pressure and temperature sensors.

### Upload

```bash
cd SmartFall/tests/BMP280
arduino-cli compile --fqbn $BOARD_FQBN .
arduino-cli upload -p PORT --fqbn $BOARD_FQBN .
arduino-cli monitor -p PORT -c baudrate=115200
```

### Expected Output

```
────────────────────────────────
    BMP280 Sensor Test
────────────────────────────────

Initializing BMP280...
✓ BMP280 initialized successfully

Reading sensor data:

Pressure:    101325 Pa (1013.25 hPa at sea level)
Temperature: 21.5°C
Altitude:    0.0 m (above sea level)

Test Result: ✓ PASS
```

### Verification Checklist

- [ ] Initialization succeeds
- [ ] Pressure reading 300-1100 hPa
- [ ] Altitude reading matches actual elevation
- [ ] Temperature ±5°C from actual room temperature
- [ ] Readings stable when stationary

### Troubleshooting

**"Failed to initialize BMP280"**
- Verify I2C address: `0x76` or `0x77`
- Check Address Select (ADDR) pin

**Wrong altitude calculation**
- Set sea-level pressure for your elevation
- Use weather service for reference pressure

## MAX30102 (Heart Rate & SpO2) Test

Tests optical heart rate sensor.

### Upload

```bash
cd SmartFall/tests/MAX30102
arduino-cli compile --fqbn $BOARD_FQBN .
arduino-cli upload -p PORT --fqbn $BOARD_FQBN .
arduino-cli monitor -p PORT -c baudrate=115200
```

### Expected Output

```
────────────────────────────────
 MAX30102 Heart Rate Sensor Test
────────────────────────────────

Initializing MAX30102...
✓ MAX30102 initialized successfully
✓ Reset pin working

Place your finger on the sensor...
Waiting for valid reading (10-20 seconds)...

✓ Valid reading acquired!

Heart Rate: 72 BPM
SpO2: 98%
Temperature: 36.8°C

Test Result: ✓ PASS
```

### Verification Checklist

- [ ] Initialization succeeds
- [ ] Reset pin responds (GPIO 21)
- [ ] After 10-20 seconds, valid readings appear
- [ ] Heart rate 40-200 BPM (reasonable range)
- [ ] SpO2 85-100% (80%+ is healthy)
- [ ] Temperature readings present

### Testing Instructions

1. Wait for "Place your finger..." message
2. Place finger directly on sensor
3. Apply light pressure (not too hard)
4. Keep finger still for 20 seconds
5. Watch for heart rate to stabilize

!!! warning "Sensor Stabilization"
    MAX30102 needs 10-20 seconds to acquire signal. Do not remove finger during this time.

## FSR (Force Sensor) Test

Tests force-sensitive resistor input.

### Upload

```bash
cd SmartFall/tests/FSR
arduino-cli compile --fqbn $BOARD_FQBN .
arduino-cli upload -p PORT --fqbn $BOARD_FQBN .
arduino-cli monitor -p PORT -c baudrate=115200
```

### Expected Output

```
────────────────────────────────
    FSR Sensor Test
────────────────────────────────

Initializing FSR sensor...
✓ FSR initialized successfully

Reading sensor data:

No Pressure:    ADC=100-200   (0-0.1 kg)
Light Touch:    ADC=2000-3000  (1-2 kg)
Firm Press:     ADC=3500-4000  (5-10 kg)

Test Result: ✓ PASS
```

### Verification Checklist

- [ ] Initialization succeeds
- [ ] ADC readings 0-4095 range
- [ ] No pressure: ADC < 500
- [ ] Light touch: ADC changes
- [ ] Firm press: ADC > 2000
- [ ] Readings respond to pressure changes

### Testing Instructions

1. Observe baseline (no pressure)
2. Apply light finger touch
3. Apply firm pressure
4. Release pressure
5. Verify readings change with pressure

## WiFi Test

Tests WiFi connectivity and HTTP transmission.

### Configuration

Edit test before uploading:

```cpp
// In WiFi test sketch
#define WIFI_SSID      "YourNetworkName"
#define WIFI_PASSWORD  "YourPassword"
#define SERVER_URL     "http://your-server.com:3000"
```

### Upload

```bash
cd SmartFall/tests/WiFi
arduino-cli compile --fqbn $BOARD_FQBN .
arduino-cli upload -p PORT --fqbn $BOARD_FQBN .
arduino-cli monitor -p PORT -c baudrate=115200
```

### Expected Output

```
────────────────────────────────
     WiFi Connectivity Test
────────────────────────────────

Scanning WiFi networks...
Found 5 networks

Connecting to "YourNetworkName"...
WiFi connected!
IP Address: 192.168.1.100
Signal Strength: -45 dBm

Testing HTTP POST to server...
Sending emergency alert...
✓ Alert transmitted successfully (200 OK)

Test Result: ✓ PASS
```

### Verification Checklist

- [ ] WiFi networks found
- [ ] Connection succeeds
- [ ] IP address assigned
- [ ] Signal strength reasonable (-30 to -70 dBm)
- [ ] HTTP POST succeeds
- [ ] Server response 200 OK

## BLE Test

Tests Bluetooth Low Energy functionality.

### Upload

```bash
cd SmartFall/tests/BLE
arduino-cli compile --fqbn $BOARD_FQBN .
arduino-cli upload -p PORT --fqbn $BOARD_FQBN .
arduino-cli monitor -p PORT -c baudrate=115200
```

### Expected Serial Output

```
────────────────────────────────
  BLE Server Test
────────────────────────────────

Initializing BLE...
✓ BLE server started
✓ Device name: "SmartFall"
✓ Service UUID created
✓ Characteristics created

Waiting for connection...
```

### Mobile Testing

1. Open BLE scanner app (LightBlue, nRF Connect)
2. Scan for "SmartFall" device
3. Connect to device
4. View service: `4fafc201-1fb5-459e-8fcc-c5c9c331914b`
5. View 5 characteristics
6. Subscribe to Emergency Alert characteristic
7. Observe: Server sends test notifications every 10 seconds

### Verification Checklist

- [ ] Device appears in BLE scan
- [ ] Can connect to device
- [ ] Service UUID visible
- [ ] All 5 characteristics present
- [ ] Notifications received when subscribed
- [ ] Command characteristic writable

## Audio Test

Tests audio amplifier and speaker system.

### Upload

```bash
cd SmartFall/tests/Audio
arduino-cli compile --fqbn $BOARD_FQBN .
arduino-cli upload -p PORT --fqbn $BOARD_FQBN .
arduino-cli monitor -p PORT -c baudrate=115200
```

### Expected Output

```
────────────────────────────────
     Audio System Test
────────────────────────────────

Initializing audio...
✓ PWM configured (GPIO 25, 5kHz)

Playing startup melody... [listening...]
Testing volume levels:
  25% volume... [quiet]
  50% volume... [normal]
  75% volume... [loud]
 100% volume... [loudest]

Playing alert patterns:
  ✓ Single beep
  ✓ Double beep
  ✓ Triple beep
  ✓ Siren pattern
  ✓ SOS Morse code
  ✓ Voice alerts

Test Result: ✓ PASS
```

### Verification Checklist

- [ ] Startup melody plays
- [ ] Volume levels audible
- [ ] All patterns produce sound
- [ ] Sound quality clear (not distorted)
- [ ] SOS Morse pattern recognizable
- [ ] Voice patterns distinct

### Troubleshooting

**No sound**
- Check GPIO 25 PWM signal with oscilloscope
- Verify PAM8302 power (3.3V)
- Check speaker connections
- Verify speaker impedance (4-8Ω)

**Distorted sound**
- Lower volume to 50-75%
- Add 100µF capacitor to PAM8302 power
- Use 8Ω speaker instead of 4Ω

## Test Execution Checklist

For complete system verification:

- [ ] MPU6050 accelerometer/gyroscope working
- [ ] BMP280 pressure and temperature accurate
- [ ] MAX30102 heart rate readings stable
- [ ] FSR pressure detection responsive
- [ ] WiFi connects and transmits
- [ ] BLE advertises and accepts commands
- [ ] Audio produces clear alert sounds
- [ ] All sensors read continuously
- [ ] No errors in serial output

## Quick Test Script

Run all tests automatically:

```bash
#!/bin/bash
TESTS="MPU6050 BMP280 MAX30102 FSR WiFi BLE Audio"
BOARD="esp32:esp32:adafruit_feather_esp32_v2"
PORT="/dev/ttyUSB0"

for TEST in $TESTS; do
  echo "Testing $TEST..."
  cd SmartFall/tests/$TEST
  arduino-cli compile --fqbn $BOARD .
  arduino-cli upload -p $PORT --fqbn $BOARD .
  echo "✓ $TEST uploaded. Check Serial Monitor."
  sleep 30
done
```

## Next Steps

1. **Fall Simulation**: See [Fall Simulation Tests](fall-simulation.md)
2. **Troubleshooting**: See [Troubleshooting Guide](../troubleshooting.md)
3. **Main System**: Upload [SmartFall.ino](../getting-started/quick-start.md)
