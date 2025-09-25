/*
  ESP32 + MPU6050 (Accel + Gyro) – Real + Simulated Motion

  ────────────── WIRING (matches Wokwi diagram) ──────────────
  Sensor: MPU6050 breakout (has both accelerometer & gyroscope)
    - VCC  -> ESP32 3V3
    - GND  -> ESP32 GND
    - SDA  -> ESP32 GPIO21   (I2C SDA)
    - SCL  -> ESP32 GPIO22   (I2C SCL)

  Pushbutton (to switch modes)
    - One leg -> ESP32 GPIO5
    - Other leg -> ESP32 GND
    (We use INPUT_PULLUP, so pressing the button pulls GPIO5 LOW)

  ────────────── LIBRARIES (libraries.txt) ──────────────
    Adafruit MPU6050
    Adafruit Unified Sensor
    Adafruit BusIO

  ────────────── HOW TO USE ──────────────
  - Open Serial Monitor (115200).
  - Press the on-screen/button GPIO5 to cycle:
      REAL → STILL → WALK → FALL_FWD → FALL_SIDE → FALL_BACK → REAL → …
  - In REAL mode, values come from the (virtual) MPU6050.
  - In other modes, we synthesize realistic accelerometer/gyro signals,
    including sensor noise and slow gyro drift.
*/

#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <math.h>

// ---------- Pins ----------
const int SDA_PIN = 21;   // I2C SDA
const int SCL_PIN = 22;   // I2C SCL
const int BTN_PIN = 5;    // pushbutton to GND (INPUT_PULLUP)

Adafruit_MPU6050 mpu;

// Modes for the simulator
enum Mode { REAL, STILL, WALK, FALL_FWD, FALL_SIDE, FALL_BACK };
Mode mode = REAL;                 // start in REAL (sensor) mode
unsigned long scenarioStart = 0;  // start time for simulated scenarios

// Button debounce state
bool lastBtn = HIGH, btnState = HIGH;
unsigned long lastDebounce = 0;

// Slow gyro drift (deg/s) – keeps changing slightly over time
float driftGx = 0, driftGy = 0, driftGz = 0;
unsigned long lastDriftUpdate = 0;

// Math helpers
const float TWO_PI_F = 6.28318530718f;
const float G = 9.80665f; // 1 g in m/s^2, to convert accel to "g"

// --------- Gaussian noise using Box–Muller ---------
float randn(float sigma) {
  float u1 = (random(1, 10000) / 10000.0f); // (0,1)
  float u2 = (random(1, 10000) / 10000.0f); // (0,1)
  float z0 = sqrtf(-2.0f * logf(u1)) * cosf(TWO_PI_F * u2);
  return z0 * sigma;
}

// Pretty names for the modes
const char* modeName(Mode m) {
  switch (m) {
    case REAL:      return "REAL";
    case STILL:     return "STILL";
    case WALK:      return "WALK";
    case FALL_FWD:  return "FALL_FORWARD";
    case FALL_SIDE: return "FALL_SIDE";
    case FALL_BACK: return "FALL_BACK";
  }
  return "?";
}

void setup() {
  Serial.begin(115200);
  Wire.begin(SDA_PIN, SCL_PIN);       // ESP32 I2C pins
  pinMode(BTN_PIN, INPUT_PULLUP);     // button to GND

  if (!mpu.begin(0x68, &Wire)) {
    Serial.println("MPU6050 not found :(");
    while (1) delay(10);
  }
  Serial.println("MPU6050 connected!");

  // Configure ranges/filters – reasonable defaults
  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
  delay(100);

  Serial.println("Press button to cycle modes: REAL → STILL → WALK → FALL_FWD → FALL_SIDE → FALL_BACK");
}

// Debounced button: a short LOW pulse on GPIO5 advances the mode
void handleButton() {
  bool reading = digitalRead(BTN_PIN);
  if (reading != lastBtn) lastDebounce = millis();

  if (millis() - lastDebounce > 50) {      // 50 ms debounce
    if (btnState != reading) {
      btnState = reading;
      if (btnState == LOW) {               // pressed (because of pull-up)
        mode = (Mode)((mode + 1) % 6);     // next mode
        scenarioStart = millis();          // restart simulated timeline
        Serial.print("Mode -> "); Serial.println(modeName(mode));
      }
    }
  }
  lastBtn = reading;
}

// Slow random walk on each gyro axis to mimic drift
void updateDrift() {
  if (millis() - lastDriftUpdate > 500) {
    driftGx += randn(0.2);  // small steps
    driftGy += randn(0.2);
    driftGz += randn(0.2);
    lastDriftUpdate = millis();
  }
}

/*
  synthIMU:
  Produces synthetic accelerometer (in g) and gyro (deg/s)
  signals depending on the current Mode.

  STILL   : 1g on Z, small noise everywhere, near-zero rotation.
  WALK    : ~1.8 Hz step pattern: Z bounces, Y sways, rotational wiggle.
  FALL_*  : freefall (low |accel|), short impact spike, then settling.
*/
void synthIMU(float &ax_g, float &ay_g, float &az_g,
              float &gx_dps, float &gy_dps, float &gz_dps) {
  updateDrift();
  float tsec = (millis() - scenarioStart) / 1000.0f;

  // Baseline = “almost still”
  ax_g = randn(0.02);
  ay_g = randn(0.02);
  az_g = 1.0f + randn(0.02);

  gx_dps = driftGx + randn(0.5);
  gy_dps = driftGy + randn(0.5);
  gz_dps = driftGz + randn(0.5);

  if (mode == WALK) {
    // ~1.8 steps per second
    const float f = 1.8f;
    float step = 0.25f * sinf(TWO_PI_F * f * tsec); // vertical bounce
    az_g = 1.0f + step + randn(0.03);

    // A gentle side sway (larger Y so you can “see the Y axis” clearly)
    ay_g += 0.15f * sinf(TWO_PI_F * f * tsec + 1.57f);

    // Some rotational motion while walking
    gx_dps += 8.0f * sinf(TWO_PI_F * f * tsec);
    gz_dps += 5.0f * cosf(TWO_PI_F * f * tsec);
    return; // done
  }

  if (mode == STILL) return; // baseline already set

  // FALL sequences:
  // 0.00–0.20s : freefall (|accel| small)
  // 0.20–0.26s : impact spike (axis depends on direction)
  // 0.26–2.50s : settle to still
  if (mode == FALL_FWD || mode == FALL_SIDE || mode == FALL_BACK) {
    if (tsec < 0.20f) {
      // Freefall: low net acceleration and some tumbling
      ax_g = randn(0.05);
      ay_g = randn(0.05);
      az_g = 0.2f + randn(0.05);
      gx_dps += randn(5.0);
      gy_dps += randn(5.0);
      gz_dps += randn(5.0);

    } else if (tsec < 0.26f) {
      // Impact spike: axis depends on the fall direction
      if (mode == FALL_FWD) {
        // forward: Z goes negative briefly, big X rotation
        az_g = -3.0f + randn(0.2);
        gx_dps += 250.0f + randn(20.0);
      }
      if (mode == FALL_SIDE) {
        // sideways: Y shows a strong positive spike, big Y rotation
        ay_g =  3.0f + randn(0.2);
        gy_dps += 220.0f + randn(20.0);
      }
      if (mode == FALL_BACK) {
        // backward: Z positive spike, big negative X rotation
        az_g =  3.0f + randn(0.2);
        gx_dps -= 240.0f + randn(20.0);
      }

    } else if (tsec < 2.50f) {
      // Settling: back to near-still with small noise
      ax_g = randn(0.01);
      ay_g = randn(0.01);
      az_g = 1.0f + randn(0.02);
      gx_dps = randn(0.5);
      gy_dps = randn(0.5);
      gz_dps = randn(0.5);

    } else {
      // Automatically go to STILL after the sequence
      mode = STILL;
      scenarioStart = millis();
    }
  }
}

// One compact line of output for logging/CSV capture
void printIMU(float ax_g, float ay_g, float az_g,
              float gx_dps, float gy_dps, float gz_dps) {
  Serial.print(modeName(mode)); Serial.print(" | ");
  Serial.print("Accel[g] X: "); Serial.print(ax_g, 3);
  Serial.print(" Y: ");       Serial.print(ay_g, 3);
  Serial.print(" Z: ");       Serial.print(az_g, 3);
  Serial.print("  |  Gyro[deg/s] X: "); Serial.print(gx_dps, 1);
  Serial.print(" Y: ");                 Serial.print(gy_dps, 1);
  Serial.print(" Z: ");                 Serial.println(gz_dps, 1);
}

void loop() {
  handleButton();

  if (mode == REAL) {
    // Read actual sensor and convert units
    sensors_event_t accel, gyro, tempEvent;
    mpu.getEvent(&accel, &gyro, &tempEvent);

    float ax = accel.acceleration.x / G;     // m/s^2 → g
    float ay = accel.acceleration.y / G;
    float az = accel.acceleration.z / G;

    float gx = gyro.gyro.x * 57.2958f;       // rad/s → deg/s
    float gy = gyro.gyro.y * 57.2958f;
    float gz = gyro.gyro.z * 57.2958f;

    printIMU(ax, ay, az, gx, gy, gz);

  } else {
    // Produce synthetic data for the selected scenario
    float ax, ay, az, gx, gy, gz;
    synthIMU(ax, ay, az, gx, gy, gz);
    printIMU(ax, ay, az, gx, gy, gz);
  }

  delay(100); // 10 Hz output
}
