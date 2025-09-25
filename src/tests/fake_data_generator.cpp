#include "fake_data_generator.h"
#include "../utils/config.h"
#include <math.h>

// Conditionally enable/disable test serial output
#if ENABLE_TEST_SERIAL_OUTPUT
  #define TEST_SERIAL_PRINT(...) Serial.print(__VA_ARGS__)
  #define TEST_SERIAL_PRINTLN(...) Serial.println(__VA_ARGS__)
#else
  #define TEST_SERIAL_PRINT(...) do {} while(0)
  #define TEST_SERIAL_PRINTLN(...) do {} while(0)
#endif

FakeDataGenerator::FakeDataGenerator() : current_scenario(SCENARIO_NORMAL_ACTIVITY),
                                         current_phase(PHASE_NORMAL),
                                         simulation_start_time(0),
                                         phase_start_time(0),
                                         scenario_duration_ms(30000),
                                         baseline_pressure(1013.25f),
                                         baseline_heart_rate(70.0f),
                                         baseline_fsr(100),
                                         accel_noise_level(0.05f),
                                         gyro_noise_level(2.0f),
                                         pressure_noise_level(0.5f),
                                         heart_rate_noise_level(3.0f),
                                         fsr_noise_level(20),
                                         fall_height_m(1.5f),
                                         impact_severity(1.0f),
                                         user_conscious(true),
                                         free_fall_duration_ms(300),
                                         impact_duration_ms(200),
                                         rotation_duration_ms(500),
                                         inactivity_duration_ms(3000) {

    // Initialize baseline accelerometer (device worn upright)
    baseline_accel[0] = 0.0f;   // x-axis
    baseline_accel[1] = 0.0f;   // y-axis
    baseline_accel[2] = 1.0f;   // z-axis (gravity)

    // Initialize baseline gyroscope (stationary)
    baseline_gyro[0] = 0.0f;
    baseline_gyro[1] = 0.0f;
    baseline_gyro[2] = 0.0f;
}

FakeDataGenerator::~FakeDataGenerator() {
    // Cleanup if needed
}

void FakeDataGenerator::startScenario(TestScenario_t scenario, uint32_t duration_ms) {
    current_scenario = scenario;
    scenario_duration_ms = duration_ms;
    simulation_start_time = millis();
    phase_start_time = simulation_start_time;
    current_phase = PHASE_NORMAL;

    // Initialize scenario-specific parameters
    switch (scenario) {
        case SCENARIO_TYPICAL_FALL:
            configureFall(1.5f, 1.0f, true);
            initializeFallScenario();
            break;

        case SCENARIO_SEVERE_FALL:
            configureFall(2.0f, 2.0f, false);
            initializeFallScenario();
            break;

        case SCENARIO_FALSE_POSITIVE_DROP:
        case SCENARIO_FALSE_POSITIVE_EXERCISE:
            initializeFalsePositiveScenario();
            break;

        case SCENARIO_WALKING:
        case SCENARIO_NORMAL_ACTIVITY:
        default:
            initializeNormalScenario();
            break;
    }

    TEST_SERIAL_PRINT("Started scenario: ");
    TEST_SERIAL_PRINT(scenario);
    TEST_SERIAL_PRINT(" for ");
    TEST_SERIAL_PRINT(duration_ms);
    TEST_SERIAL_PRINTLN(" ms");
}

void FakeDataGenerator::stopScenario() {
    current_scenario = SCENARIO_NORMAL_ACTIVITY;
    current_phase = PHASE_NORMAL;
    simulation_start_time = 0;
}

bool FakeDataGenerator::isScenarioActive() {
    if (simulation_start_time == 0) return false;
    return (millis() - simulation_start_time) < scenario_duration_ms;
}

TestScenario_t FakeDataGenerator::getCurrentScenario() {
    return current_scenario;
}

FallPhase_t FakeDataGenerator::getCurrentPhase() {
    return current_phase;
}

uint32_t FakeDataGenerator::getScenarioProgress() {
    if (!isScenarioActive()) return 100;
    uint32_t elapsed = millis() - simulation_start_time;
    return (elapsed * 100) / scenario_duration_ms;
}

void FakeDataGenerator::setBaseline(float ax, float ay, float az, float gx, float gy, float gz,
                                   float pressure, float heart_rate, uint16_t fsr) {
    baseline_accel[0] = ax;
    baseline_accel[1] = ay;
    baseline_accel[2] = az;
    baseline_gyro[0] = gx;
    baseline_gyro[1] = gy;
    baseline_gyro[2] = gz;
    baseline_pressure = pressure;
    baseline_heart_rate = heart_rate;
    baseline_fsr = fsr;
}

void FakeDataGenerator::setNoiseLevel(float accel_noise, float gyro_noise, float pressure_noise,
                                     float hr_noise, uint16_t fsr_noise) {
    accel_noise_level = accel_noise;
    gyro_noise_level = gyro_noise;
    pressure_noise_level = pressure_noise;
    heart_rate_noise_level = hr_noise;
    fsr_noise_level = fsr_noise;
}

void FakeDataGenerator::configureFall(float height_m, float severity, bool conscious) {
    fall_height_m = height_m;
    impact_severity = severity;
    user_conscious = conscious;

    // Adjust phase durations based on fall parameters
    free_fall_duration_ms = sqrt(2 * height_m / 9.81f) * 1000; // Physics-based
    impact_duration_ms = 100 + (severity * 100);
    rotation_duration_ms = 300 + (severity * 200);
    inactivity_duration_ms = conscious ? 2000 : 5000;
}

void FakeDataGenerator::setFallTiming(uint32_t free_fall_ms, uint32_t impact_ms,
                                     uint32_t rotation_ms, uint32_t inactivity_ms) {
    free_fall_duration_ms = free_fall_ms;
    impact_duration_ms = impact_ms;
    rotation_duration_ms = rotation_ms;
    inactivity_duration_ms = inactivity_ms;
}

SensorData_t FakeDataGenerator::generateSensorData() {
    if (!isScenarioActive()) {
        return generateNormalActivity();
    }

    updateCurrentPhase();

    SensorData_t data = {0};
    data.timestamp = millis();
    data.valid = true;

    switch (current_scenario) {
        case SCENARIO_TYPICAL_FALL:
        case SCENARIO_SEVERE_FALL:
            data = generateFallData();
            break;

        case SCENARIO_FALSE_POSITIVE_DROP:
        case SCENARIO_FALSE_POSITIVE_EXERCISE:
            data = generateFalsePositiveData();
            break;

        case SCENARIO_WALKING:
            data = generateWalkingData();
            break;

        case SCENARIO_NORMAL_ACTIVITY:
        default:
            data = generateNormalActivity();
            break;
    }

    return data;
}

bool FakeDataGenerator::generateSensorData(SensorData_t& data) {
    data = generateSensorData();
    return isScenarioActive();
}

SensorData_t FakeDataGenerator::generateNormalActivity() {
    SensorData_t data = {0};

    // Generate baseline accelerometer data with slight variations
    data.accel_x = baseline_accel[0];
    data.accel_y = baseline_accel[1];
    data.accel_z = baseline_accel[2];
    addNoiseToAccel(data.accel_x, data.accel_y, data.accel_z);

    // Generate baseline gyroscope data with slight variations
    data.gyro_x = baseline_gyro[0];
    data.gyro_y = baseline_gyro[1];
    data.gyro_z = baseline_gyro[2];
    addNoiseToGyro(data.gyro_x, data.gyro_y, data.gyro_z);

    // Stable environmental data
    data.pressure = addNoiseToValue(baseline_pressure, pressure_noise_level);
    data.heart_rate = addNoiseToValue(baseline_heart_rate, heart_rate_noise_level);
    data.fsr_value = addNoiseToValue(baseline_fsr, fsr_noise_level);

    data.timestamp = millis();
    data.valid = true;

    return data;
}

SensorData_t FakeDataGenerator::generateWalkingData() {
    SensorData_t data = generateNormalActivity();

    // Add walking-specific motion patterns
    float time_s = (millis() - simulation_start_time) / 1000.0f;
    float step_frequency = 1.8f; // steps per second
    float step_phase = time_s * step_frequency * 2 * PI;

    // Walking creates periodic accelerations
    data.accel_x += 0.3f * sin(step_phase);
    data.accel_y += 0.2f * cos(step_phase * 2);
    data.accel_z += 0.4f * sin(step_phase) + 1.0f;

    // Walking creates small gyroscopic movements
    data.gyro_x += 15.0f * sin(step_phase + PI/4);
    data.gyro_y += 10.0f * cos(step_phase);
    data.gyro_z += 8.0f * sin(step_phase * 1.5f);

    // Slight heart rate increase during walking
    data.heart_rate += 15.0f;

    return data;
}

SensorData_t FakeDataGenerator::generateFallData() {
    switch (current_phase) {
        case PHASE_FREE_FALL:
            return generateFreeFallPhase();
        case PHASE_IMPACT:
            return generateImpactPhase();
        case PHASE_POST_IMPACT_ROTATION:
            return generateRotationPhase();
        case PHASE_INACTIVITY:
            return generateInactivityPhase();
        case PHASE_RECOVERY:
            return generateRecoveryPhase();
        default:
            return generateNormalActivity();
    }
}

SensorData_t FakeDataGenerator::generateFalsePositiveData() {
    SensorData_t data = generateNormalActivity();

    uint32_t elapsed = millis() - simulation_start_time;

    if (current_scenario == SCENARIO_FALSE_POSITIVE_DROP) {
        // Simulate dropping the device
        if (elapsed < 500) {
            // Brief free-fall like acceleration
            data.accel_x = 0.1f;
            data.accel_y = 0.1f;
            data.accel_z = 0.2f; // Low but not true free fall
        } else if (elapsed < 700) {
            // Impact when device hits ground
            data.accel_x = 2.5f; // Below fall threshold
            data.accel_y = 1.8f;
            data.accel_z = 2.2f;
        }
        // No prolonged inactivity - device is picked up
    } else if (current_scenario == SCENARIO_FALSE_POSITIVE_EXERCISE) {
        // Simulate vigorous exercise movements
        float time_s = elapsed / 1000.0f;
        float exercise_frequency = 3.0f;
        float phase = time_s * exercise_frequency * 2 * PI;

        data.accel_x = 1.5f * sin(phase);
        data.accel_y = 1.2f * cos(phase * 1.3f);
        data.accel_z = 1.0f + 0.8f * sin(phase * 0.8f);

        data.gyro_x = 45.0f * sin(phase + PI/3);
        data.gyro_y = 35.0f * cos(phase * 1.2f);
        data.gyro_z = 40.0f * sin(phase * 0.9f);

        // Elevated heart rate during exercise
        data.heart_rate += 40.0f;
    }

    return data;
}

SensorData_t FakeDataGenerator::generateFreeFallPhase() {
    SensorData_t data = {0};

    // Free fall: very low acceleration (weightlessness)
    data.accel_x = gaussianRandom(0.0f, 0.1f);
    data.accel_y = gaussianRandom(0.0f, 0.1f);
    data.accel_z = gaussianRandom(0.2f, 0.15f); // Slightly above zero due to air resistance

    // Moderate rotation during fall
    data.gyro_x = gaussianRandom(0.0f, 50.0f);
    data.gyro_y = gaussianRandom(0.0f, 50.0f);
    data.gyro_z = gaussianRandom(0.0f, 30.0f);

    // Environment data remains stable initially
    data.pressure = baseline_pressure;
    data.heart_rate = baseline_heart_rate + 10.0f; // Slight increase due to stress
    data.fsr_value = baseline_fsr;

    data.timestamp = millis();
    data.valid = true;

    return data;
}

SensorData_t FakeDataGenerator::generateImpactPhase() {
    SensorData_t data = {0};

    // High impact accelerations
    float impact_multiplier = impact_severity;
    data.accel_x = gaussianRandom(0.0f, 2.0f) * impact_multiplier;
    data.accel_y = gaussianRandom(0.0f, 2.0f) * impact_multiplier;
    data.accel_z = gaussianRandom(4.0f, 1.5f) * impact_multiplier; // Primary impact direction

    // High angular velocity during impact
    data.gyro_x = gaussianRandom(0.0f, 100.0f);
    data.gyro_y = gaussianRandom(0.0f, 100.0f);
    data.gyro_z = gaussianRandom(0.0f, 80.0f);

    // FSR spike if device impacts ground
    data.fsr_value = baseline_fsr + (500 * impact_severity);

    // Pressure change if significant altitude change
    data.pressure = baseline_pressure - (fall_height_m * 0.12f); // ~0.12 hPa per meter

    // Heart rate spike from stress/pain
    data.heart_rate = baseline_heart_rate + (20.0f * impact_severity);

    data.timestamp = millis();
    data.valid = true;

    return data;
}

SensorData_t FakeDataGenerator::generateRotationPhase() {
    SensorData_t data = {0};

    // Continued rotation after impact
    float rotation_decay = 1.0f - (getPhaseProgress() * 0.7f); // Rotation decays over time

    data.gyro_x = gaussianRandom(0.0f, 150.0f) * rotation_decay;
    data.gyro_y = gaussianRandom(0.0f, 200.0f) * rotation_decay;
    data.gyro_z = gaussianRandom(0.0f, 100.0f) * rotation_decay;

    // Acceleration settles toward new orientation
    float settle_progress = getPhaseProgress();
    data.accel_x = smoothTransition(gaussianRandom(0.0f, 1.0f), 0.3f, settle_progress);
    data.accel_y = smoothTransition(gaussianRandom(0.0f, 1.0f), -0.2f, settle_progress);
    data.accel_z = smoothTransition(gaussianRandom(0.0f, 1.0f), 0.9f, settle_progress);

    // Stable environmental readings
    data.pressure = baseline_pressure - (fall_height_m * 0.12f);
    data.heart_rate = baseline_heart_rate + (15.0f * impact_severity);
    data.fsr_value = baseline_fsr + (100 * impact_severity);

    data.timestamp = millis();
    data.valid = true;

    return data;
}

SensorData_t FakeDataGenerator::generateInactivityPhase() {
    SensorData_t data = {0};

    // Very stable accelerometer readings (person lying still)
    data.accel_x = gaussianRandom(0.2f, 0.05f);
    data.accel_y = gaussianRandom(-0.1f, 0.05f);
    data.accel_z = gaussianRandom(0.95f, 0.05f); // Close to 1g in new orientation

    // Minimal rotation (person not moving)
    data.gyro_x = gaussianRandom(0.0f, 5.0f);
    data.gyro_y = gaussianRandom(0.0f, 5.0f);
    data.gyro_z = gaussianRandom(0.0f, 3.0f);

    // Continued stress response in heart rate
    float stress_level = user_conscious ? 1.0f : 0.5f;
    data.heart_rate = baseline_heart_rate + (25.0f * stress_level);

    // Stable environmental readings
    data.pressure = baseline_pressure - (fall_height_m * 0.12f);
    data.fsr_value = baseline_fsr + (50 * impact_severity);

    data.timestamp = millis();
    data.valid = true;

    return data;
}

SensorData_t FakeDataGenerator::generateRecoveryPhase() {
    SensorData_t data = {0};

    float recovery_progress = getPhaseProgress();

    if (user_conscious && recovery_progress > 0.3f) {
        // User attempting to get up
        data.accel_x = gaussianRandom(0.0f, 0.5f) * (1.0f - recovery_progress);
        data.accel_y = gaussianRandom(0.0f, 0.5f) * (1.0f - recovery_progress);
        data.accel_z = smoothTransition(0.95f, 1.0f, recovery_progress);

        // Movement during recovery attempt
        data.gyro_x = gaussianRandom(0.0f, 30.0f) * (1.0f - recovery_progress);
        data.gyro_y = gaussianRandom(0.0f, 40.0f) * (1.0f - recovery_progress);
        data.gyro_z = gaussianRandom(0.0f, 25.0f) * (1.0f - recovery_progress);

        // Heart rate gradually returning to baseline
        data.heart_rate = smoothTransition(baseline_heart_rate + 25.0f, baseline_heart_rate + 5.0f, recovery_progress);
    } else {
        // No recovery - remain inactive
        return generateInactivityPhase();
    }

    data.pressure = baseline_pressure - (fall_height_m * 0.12f);
    data.fsr_value = baseline_fsr;

    data.timestamp = millis();
    data.valid = true;

    return data;
}

void FakeDataGenerator::addNoiseToAccel(float& x, float& y, float& z) {
    x += gaussianRandom(0.0f, accel_noise_level);
    y += gaussianRandom(0.0f, accel_noise_level);
    z += gaussianRandom(0.0f, accel_noise_level);
}

void FakeDataGenerator::addNoiseToGyro(float& x, float& y, float& z) {
    x += gaussianRandom(0.0f, gyro_noise_level);
    y += gaussianRandom(0.0f, gyro_noise_level);
    z += gaussianRandom(0.0f, gyro_noise_level);
}

float FakeDataGenerator::addNoiseToValue(float value, float noise_level) {
    return value + gaussianRandom(0.0f, noise_level);
}

uint16_t FakeDataGenerator::addNoiseToValue(uint16_t value, uint16_t noise_level) {
    int noisy_value = value + (int)gaussianRandom(0.0f, noise_level);
    return constrain(noisy_value, 0, 4095);
}

bool FakeDataGenerator::validateGeneratedData(const SensorData_t& data) {
    // Basic sanity checks
    if (!data.valid) return false;
    if (data.heart_rate < 30.0f || data.heart_rate > 200.0f) return false;
    if (data.pressure < 800.0f || data.pressure > 1200.0f) return false;
    if (data.fsr_value > 4095) return false;

    float total_accel = sqrt(data.accel_x*data.accel_x + data.accel_y*data.accel_y + data.accel_z*data.accel_z);
    if (total_accel > 10.0f) return false; // Sanity check for extreme values

    return true;
}

void FakeDataGenerator::updateCurrentPhase() {
    if (!isScenarioActive() ||
        (current_scenario != SCENARIO_TYPICAL_FALL && current_scenario != SCENARIO_SEVERE_FALL)) {
        return;
    }

    uint32_t scenario_elapsed = millis() - simulation_start_time;

    // Define phase transitions for fall scenarios
    if (current_phase == PHASE_NORMAL && scenario_elapsed > 1000) {
        current_phase = PHASE_FREE_FALL;
        phase_start_time = millis();
    } else if (current_phase == PHASE_FREE_FALL && getPhaseElapsedTime() > free_fall_duration_ms) {
        current_phase = PHASE_IMPACT;
        phase_start_time = millis();
    } else if (current_phase == PHASE_IMPACT && getPhaseElapsedTime() > impact_duration_ms) {
        current_phase = PHASE_POST_IMPACT_ROTATION;
        phase_start_time = millis();
    } else if (current_phase == PHASE_POST_IMPACT_ROTATION && getPhaseElapsedTime() > rotation_duration_ms) {
        current_phase = PHASE_INACTIVITY;
        phase_start_time = millis();
    } else if (current_phase == PHASE_INACTIVITY && getPhaseElapsedTime() > inactivity_duration_ms) {
        current_phase = PHASE_RECOVERY;
        phase_start_time = millis();
    }
}

uint32_t FakeDataGenerator::getPhaseElapsedTime() {
    return millis() - phase_start_time;
}

float FakeDataGenerator::getPhaseProgress() {
    uint32_t elapsed = getPhaseElapsedTime();
    uint32_t phase_duration = 0;

    switch (current_phase) {
        case PHASE_FREE_FALL: phase_duration = free_fall_duration_ms; break;
        case PHASE_IMPACT: phase_duration = impact_duration_ms; break;
        case PHASE_POST_IMPACT_ROTATION: phase_duration = rotation_duration_ms; break;
        case PHASE_INACTIVITY: phase_duration = inactivity_duration_ms; break;
        case PHASE_RECOVERY: phase_duration = 5000; break; // 5 second recovery window
        default: return 0.0f;
    }

    if (phase_duration == 0) return 1.0f;
    return constrain((float)elapsed / phase_duration, 0.0f, 1.0f);
}

float FakeDataGenerator::gaussianRandom(float mean, float stddev) {
    static bool has_spare = false;
    static float spare;

    if (has_spare) {
        has_spare = false;
        return spare * stddev + mean;
    }

    has_spare = true;
    static float u, v, mag;
    do {
        u = random(0, 32767) / 16383.5f - 1.0f;
        v = random(0, 32767) / 16383.5f - 1.0f;
        mag = u * u + v * v;
    } while (mag >= 1.0f || mag == 0.0f);

    mag = sqrt(-2.0f * log(mag) / mag);
    spare = v * mag;
    return mean + stddev * u * mag;
}

float FakeDataGenerator::smoothTransition(float from, float to, float progress) {
    progress = constrain(progress, 0.0f, 1.0f);
    // Smooth cubic transition
    float smooth_progress = progress * progress * (3.0f - 2.0f * progress);
    return from + (to - from) * smooth_progress;
}

void FakeDataGenerator::initializeFallScenario() {
    // Fall scenarios start with normal activity, then transition to fall phases
    current_phase = PHASE_NORMAL;
}

void FakeDataGenerator::initializeNormalScenario() {
    current_phase = PHASE_NORMAL;
}

void FakeDataGenerator::initializeFalsePositiveScenario() {
    current_phase = PHASE_NORMAL;
}

void FakeDataGenerator::printCurrentScenarioStatus() {
    TEST_SERIAL_PRINT("Scenario: ");
    TEST_SERIAL_PRINT(current_scenario);
    TEST_SERIAL_PRINT(" | Phase: ");
    TEST_SERIAL_PRINT(current_phase);
    TEST_SERIAL_PRINT(" | Progress: ");
    TEST_SERIAL_PRINT(getScenarioProgress());
    TEST_SERIAL_PRINTLN("%");
}

void FakeDataGenerator::printGeneratedData(const SensorData_t& data) {
    TEST_SERIAL_PRINT("Accel: ");
    TEST_SERIAL_PRINT(data.accel_x, 2); TEST_SERIAL_PRINT(",");
    TEST_SERIAL_PRINT(data.accel_y, 2); TEST_SERIAL_PRINT(",");
    TEST_SERIAL_PRINT(data.accel_z, 2);
    TEST_SERIAL_PRINT(" | Gyro: ");
    TEST_SERIAL_PRINT(data.gyro_x, 1); TEST_SERIAL_PRINT(",");
    TEST_SERIAL_PRINT(data.gyro_y, 1); TEST_SERIAL_PRINT(",");
    TEST_SERIAL_PRINT(data.gyro_z, 1);
    TEST_SERIAL_PRINT(" | HR: ");
    TEST_SERIAL_PRINT(data.heart_rate, 0);
    TEST_SERIAL_PRINT(" | P: ");
    TEST_SERIAL_PRINT(data.pressure, 1);
    TEST_SERIAL_PRINT(" | FSR: ");
    TEST_SERIAL_PRINTLN(data.fsr_value);
}

// TestDataSets implementation
void TestDataSets::setupTypicalFall(FakeDataGenerator& generator) {
    generator.configureFall(1.5f, 1.0f, true);
    generator.setFallTiming(300, 150, 500, 2500);
    generator.setNoiseLevel(0.02f, 1.0f, 0.3f, 2.0f, 15);
}

void TestDataSets::setupSevereFall(FakeDataGenerator& generator) {
    generator.configureFall(2.5f, 2.0f, false);
    generator.setFallTiming(400, 300, 800, 5000);
    generator.setNoiseLevel(0.03f, 2.0f, 0.5f, 5.0f, 25);
}

void TestDataSets::setupFalsePositiveDrop(FakeDataGenerator& generator) {
    generator.configureFall(0.5f, 0.3f, true);
    generator.setFallTiming(100, 50, 200, 500);
    generator.setNoiseLevel(0.01f, 0.5f, 0.1f, 1.0f, 10);
}

void TestDataSets::setupFalsePositiveExercise(FakeDataGenerator& generator) {
    generator.setBaseline(0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1013.25f, 85.0f, 120);
    generator.setNoiseLevel(0.1f, 5.0f, 1.0f, 8.0f, 30);
}

void TestDataSets::setupNormalActivity(FakeDataGenerator& generator) {
    generator.setBaseline(0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1013.25f, 70.0f, 100);
    generator.setNoiseLevel(0.02f, 1.0f, 0.2f, 2.0f, 15);
}

void TestDataSets::setupWalkingActivity(FakeDataGenerator& generator) {
    generator.setBaseline(0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1013.25f, 85.0f, 110);
    generator.setNoiseLevel(0.05f, 3.0f, 0.3f, 3.0f, 20);
}