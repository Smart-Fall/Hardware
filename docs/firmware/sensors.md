# Sensor Modules

Detailed documentation of sensor drivers and APIs.

## MPU6050 (6-Axis IMU)

**Measures**: Acceleration and angular velocity

### Hardware Specs

| Parameter | Value |
|-----------|-------|
| **Interface** | I2C (0x68) |
| **Axes** | 6 (3 accel + 3 gyro) |
| **Sample Rate** | 100-1000 Hz |
| **Accel Range** | ±2g, ±4g, ±8g, ±16g |
| **Gyro Range** | ±250°/s, ±500°/s, ±1000°/s, ±2000°/s |
| **Power** | ~3.9 mA continuous |

### API Usage

```cpp
#include "MPU6050.h"

MPU6050 mpu;

// Initialize
bool success = mpu.init();
if (!success) {
    Serial.println("MPU6050 initialization failed");
}

// Read latest data
float accel_x, accel_y, accel_z;      // m/s²
float gyro_x, gyro_y, gyro_z;         // °/s
mpu.readAccel(&accel_x, &accel_y, &accel_z);
mpu.readGyro(&gyro_x, &gyro_y, &gyro_z);

// Calculate total acceleration
float total_accel = sqrt(accel_x*accel_x +
                        accel_y*accel_y +
                        accel_z*accel_z);

// Calculate angular magnitude
float angular_mag = sqrt(gyro_x*gyro_x +
                        gyro_y*gyro_y +
                        gyro_z*gyro_z);
```

### Configuration

```cpp
// In Config.h
#define MPU6050_SDA_PIN SDA   // GPIO 22
#define MPU6050_SCL_PIN SCL   // GPIO 20
```

### Typical Readings

| Movement | Accel | Gyro |
|----------|-------|------|
| Static | ~1g (gravity) | ~0°/s |
| Walking | 0.8-1.2g | 50-100°/s |
| Running | 1.2-1.5g | 100-200°/s |
| Falling | <0.5g (free fall) | 200-600°/s (rotating) |
| Impact | >3g | 0°/s (at ground) |

## BMP280 (Pressure & Temperature)

**Measures**: Barometric pressure, temperature, and altitude

### Hardware Specs

| Parameter | Value |
|-----------|-------|
| **Interface** | I2C (0x76 or 0x77) |
| **Pressure Range** | 300-1100 hPa |
| **Altitude Accuracy** | ±100m |
| **Temperature Range** | -40 to 85°C |
| **Power** | ~0.7 mA |

### API Usage

```cpp
#include "BMP280.h"

BMP280 bmp;

// Initialize
bool success = bmp.init();

// Read pressure (Pascal)
float pressure_pa = bmp.readPressure();

// Read temperature (Celsius)
float temp_c = bmp.readTemperature();

// Calculate altitude (meters above sea level)
float altitude_m = bmp.readAltitude(101325.0);  // Sea level reference

// Or calculate altitude change from baseline
float baseline_pressure = 101325;  // hPa at sea level
float altitude_change = bmp.calculateAltitudeChange(baseline_pressure);
```

### Configuration

```cpp
// In Config.h
#define BMP280_SDA_PIN SDA    // GPIO 22
#define BMP280_SCL_PIN SCL    // GPIO 20
```

### Fall Detection Usage

- **Altitude drop during fall**: Indicates real fall vs device drop
- **Typical drop**: 0.5-2 meters during short fall
- **Timing**: Update every 1-2 seconds (not real-time)

## MAX30102 (Heart Rate & SpO2)

**Measures**: Heart rate (BPM) and blood oxygen saturation (SpO2)

### Hardware Specs

| Parameter | Value |
|-----------|-------|
| **Interface** | I2C (0x57) |
| **Reset Pin** | GPIO 21 (MI) |
| **Sensors** | Dual LED (Red + IR) |
| **Update Rate** | ~4 seconds |
| **Power** | ~10 mA continuous |

### API Usage

```cpp
#include "MAX30102_Sensor.h"

MAX30102Sensor max30102;

// Initialize
bool success = max30102.init();

// Read heart rate (BPM)
uint16_t heart_rate = max30102.getHeartRate();

// Read oxygen saturation (%)
uint8_t spo2 = max30102.getSPO2();

// Read temperature (°C)
float temperature = max30102.getTemperature();

// Check if data is valid
if (max30102.isHeartRateValid()) {
    Serial.print("Heart Rate: ");
    Serial.print(heart_rate);
    Serial.println(" BPM");
}
```

### Configuration

```cpp
// In Config.h
#define MAX30102_SDA_PIN SDA     // GPIO 22
#define MAX30102_SCL_PIN SCL     // GPIO 20
#define MAX30102_RST_PIN A9      // GPIO 21 (hardware reset)
```

### Physiological Indicators

| HR Change | Confidence Bonus | Notes |
|-----------|-----------------|-------|
| **≥40 BPM increase** | +8 points | Strong panic/stress response |
| **20-40 BPM increase** | +5 points | Moderate response |
| **<20 BPM increase** | 0 points | No significant change |
| **SpO2 ≥90%** | +5 points | Healthy oxygen saturation |
| **SpO2 85-90%** | +2 points | Slightly reduced |
| **SpO2 <85%** | -3 points | Concerning level |

### Timing Considerations

- **Sensor stabilization**: ~10 seconds after placement
- **Update interval**: ~4 seconds between readings
- **Historical context**: Compares to user baseline (learned over time)

## FSR (Force Sensitive Resistor)

**Measures**: Pressure/force on device strap

### Hardware Specs

| Parameter | Value |
|-----------|-------|
| **Interface** | Analog (ADC1, GPIO 34) |
| **Force Range** | 0-10 kg |
| **Resistance Range** | ~200kΩ (no force) to <1kΩ (10kg) |
| **Power** | <1 mA |

### API Usage

```cpp
#include "FSR.h"

FSR fsr;

// Initialize (sets up pin and pull-up)
bool success = fsr.init();

// Read raw ADC value (0-4095)
uint16_t raw_value = fsr.readRaw();

// Convert to estimated pressure (Pa)
float pressure_pa = fsr.readPressure();

// Check if pressure is in normal range (device worn)
if (fsr.isWorn()) {
    // Device attached normally
} else {
    // Device may have shifted or been removed
}
```

### ADC Conversion

```
FSR Voltage = (ADC_reading / 4095) × 3.3V

Resistance calculation:
R_fsr = (22k × V_fsr) / (3.3 - V_fsr)

Pressure estimation:
P_contact = R_fsr calibration curve (device-dependent)
```

### Configuration

```cpp
// In Config.h
#define FSR1_PIN A0    // or other ADC1 pins
#define FSR2_PIN A1    // Use only ADC1 pins (34, 35, 36, 39)
#define FSR3_PIN A2    // GPIO 34 used in this design
#define FSR4_PIN A3
```

### Fall Detection Usage

- **Impact detection**: FSR spike during impact phase
- **Strap tension**: Confirms device remains attached
- **Baseline drift**: Long-term aging compensation needed

## Sensor Data Structure

All sensors populate the shared `SensorData_t` structure:

```cpp
struct SensorData_t {
    // Timing
    uint32_t timestamp_ms;

    // MPU6050
    float accel_x, accel_y, accel_z;
    float gyro_x, gyro_y, gyro_z;

    // BMP280
    float pressure_pa;
    float altitude_m;
    float temperature_c;

    // MAX30102
    uint16_t heart_rate;
    uint8_t spo2;
    float heart_rate_temperature;

    // FSR
    uint16_t fsr_value;
    float device_pressure;

    // Power
    float battery_voltage;
    uint8_t battery_percent;
};
```

## Sensor Calibration

### MPU6050 Calibration

```cpp
// Accelerometer offsets (for static calibration)
mpu.setAccelOffset(x_offset, y_offset, z_offset);

// Gyroscope offsets (zero-rate output)
mpu.setGyroOffset(x_offset, y_offset, z_offset);
```

### BMP280 Calibration

```cpp
// Set sea-level reference pressure
float sea_level_pressure = bmp.readPressure(); // @ known elevation
bmp.setSeaLevelPressure(sea_level_pressure);
```

### MAX30102 Calibration

MAX30102 has built-in calibration. May need LED power adjustment:

```cpp
// Adjust LED brightness if readings are inconsistent
max30102.setLEDMode(MAX30102_MODE_REDONLY);
max30102.setPulseAmplitude(MAX30102_LED_BRIGHTNESS_HIGH);
```

## Sensor Integration Points

Sensors are read in the main loop and passed to fall detection:

```cpp
// In SmartFall.ino main loop:
SensorData_t current_data;
current_data.timestamp_ms = millis();

// Read all sensors
mpu.readAccel(&current_data.accel_x, &current_data.accel_y, &current_data.accel_z);
mpu.readGyro(&current_data.gyro_x, &current_data.gyro_y, &current_data.gyro_z);

current_data.pressure_pa = bmp.readPressure();
current_data.altitude_m = bmp.readAltitude();
current_data.temperature_c = bmp.readTemperature();

current_data.heart_rate = max30102.getHeartRate();
current_data.spo2 = max30102.getSPO2();

current_data.fsr_value = fsr.readRaw();

// Pass to fall detector
fall_detector.processSensorData(current_data);
```

## Next Steps

1. **Fall Detection**: See [Fall Detection Algorithm](fall-detection.md)
2. **Testing**: See [Component Tests](../testing/component-tests.md)
3. **Configuration**: See [Config Reference](../configuration/config-reference.md)
4. **Troubleshooting**: See [Troubleshooting Guide](../troubleshooting.md)
