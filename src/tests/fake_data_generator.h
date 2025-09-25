#ifndef FAKE_DATA_GENERATOR_H
#define FAKE_DATA_GENERATOR_H

#include "../utils/data_types.h"
#include "../utils/config.h"
#include <Arduino.h>

// Test scenario types
typedef enum {
    SCENARIO_NORMAL_ACTIVITY,
    SCENARIO_WALKING,
    SCENARIO_SITTING_DOWN,
    SCENARIO_STANDING_UP,
    SCENARIO_TYPICAL_FALL,
    SCENARIO_SEVERE_FALL,
    SCENARIO_FALSE_POSITIVE_DROP,
    SCENARIO_FALSE_POSITIVE_EXERCISE,
    SCENARIO_RECOVERY_AFTER_FALL,
    SCENARIO_CUSTOM
} TestScenario_t;

// Fall simulation phases
typedef enum {
    PHASE_NORMAL,
    PHASE_PRE_FALL,
    PHASE_FREE_FALL,
    PHASE_IMPACT,
    PHASE_POST_IMPACT_ROTATION,
    PHASE_INACTIVITY,
    PHASE_RECOVERY
} FallPhase_t;

class FakeDataGenerator {
private:
    // Current simulation parameters
    TestScenario_t current_scenario;
    FallPhase_t current_phase;
    uint32_t simulation_start_time;
    uint32_t phase_start_time;
    uint32_t scenario_duration_ms;

    // Baseline sensor values (normal activity)
    float baseline_accel[3];  // x, y, z in g
    float baseline_gyro[3];   // x, y, z in °/s
    float baseline_pressure;  // hPa
    float baseline_heart_rate; // BPM
    uint16_t baseline_fsr;    // ADC counts

    // Noise and variation parameters
    float accel_noise_level;
    float gyro_noise_level;
    float pressure_noise_level;
    float heart_rate_noise_level;
    uint16_t fsr_noise_level;

    // Fall simulation specific parameters
    float fall_height_m;      // Simulated fall height
    float impact_severity;    // 1.0 = normal, 2.0 = severe
    bool user_conscious;      // Affects recovery patterns

    // Phase timing parameters
    uint32_t free_fall_duration_ms;
    uint32_t impact_duration_ms;
    uint32_t rotation_duration_ms;
    uint32_t inactivity_duration_ms;

public:
    FakeDataGenerator();
    ~FakeDataGenerator();

    // Scenario control
    void startScenario(TestScenario_t scenario, uint32_t duration_ms = 30000);
    void stopScenario();
    bool isScenarioActive();
    TestScenario_t getCurrentScenario();
    FallPhase_t getCurrentPhase();
    uint32_t getScenarioProgress(); // Returns % complete (0-100)

    // Baseline configuration
    void setBaseline(float ax, float ay, float az, float gx, float gy, float gz,
                    float pressure, float heart_rate, uint16_t fsr);
    void setNoiseLevel(float accel_noise, float gyro_noise, float pressure_noise,
                      float hr_noise, uint16_t fsr_noise);

    // Fall-specific parameters
    void configureFall(float height_m, float severity, bool conscious = true);
    void setFallTiming(uint32_t free_fall_ms, uint32_t impact_ms,
                      uint32_t rotation_ms, uint32_t inactivity_ms);

    // Data generation
    SensorData_t generateSensorData();
    bool generateSensorData(SensorData_t& data); // Returns true if scenario still active

    // Specific scenario generators
    SensorData_t generateNormalActivity();
    SensorData_t generateWalkingData();
    SensorData_t generateFallData();
    SensorData_t generateFalsePositiveData();

    // Phase-specific generators
    SensorData_t generateFreeFallPhase();
    SensorData_t generateImpactPhase();
    SensorData_t generateRotationPhase();
    SensorData_t generateInactivityPhase();
    SensorData_t generateRecoveryPhase();

    // Utility functions
    void addNoiseToAccel(float& x, float& y, float& z);
    void addNoiseToGyro(float& x, float& y, float& z);
    float addNoiseToValue(float value, float noise_level);
    uint16_t addNoiseToValue(uint16_t value, uint16_t noise_level);

    // Test data validation
    bool validateGeneratedData(const SensorData_t& data);
    void printCurrentScenarioStatus();
    void printGeneratedData(const SensorData_t& data);

    // Advanced scenario control
    void skipToPhase(FallPhase_t phase);
    void extendScenario(uint32_t additional_ms);
    void injectCustomData(float ax, float ay, float az, float gx, float gy, float gz);

private:
    // Internal phase management
    void updateCurrentPhase();
    bool shouldTransitionPhase();
    void transitionToNextPhase();

    // Mathematical helpers
    float gaussianRandom(float mean, float stddev);
    float smoothTransition(float from, float to, float progress); // progress 0.0-1.0
    uint32_t getPhaseElapsedTime();
    float getPhaseProgress(); // Returns 0.0-1.0 for current phase

    // Scenario-specific phase timing
    void initializeFallScenario();
    void initializeNormalScenario();
    void initializeFalsePositiveScenario();
};

// Helper functions for test setup
class TestDataSets {
public:
    // Predefined test scenarios
    static void setupTypicalFall(FakeDataGenerator& generator);
    static void setupSevereFall(FakeDataGenerator& generator);
    static void setupFalsePositiveDrop(FakeDataGenerator& generator);
    static void setupFalsePositiveExercise(FakeDataGenerator& generator);
    static void setupNormalActivity(FakeDataGenerator& generator);
    static void setupWalkingActivity(FakeDataGenerator& generator);

    // Edge case scenarios
    static void setupBorderlineDetection(FakeDataGenerator& generator);
    static void setupMultipleRecoveryAttempts(FakeDataGenerator& generator);
    static void setupSensorMalfunctionScenario(FakeDataGenerator& generator);
};

#endif // FAKE_DATA_GENERATOR_H