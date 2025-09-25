#ifndef TEST_INTEGRATION_H
#define TEST_INTEGRATION_H

#include "../detection/fall_detector.h"
#include "../detection/confidence_scorer.h"
#include "../utils/data_types.h"
#include "fake_data_generator.h"
#include <Arduino.h>

// Test configuration
struct TestConfig {
    bool verbose_output;
    bool print_sensor_data;
    bool print_detection_steps;
    uint32_t max_test_duration_ms;
    uint32_t sensor_sample_rate_ms;
};

class TestIntegration {
private:
    // Core system components
    FallDetector fall_detector;
    ConfidenceScorer confidence_scorer;
    FakeDataGenerator data_generator;

    // Test tracking
    int tests_passed;
    int tests_failed;
    TestConfig config;

    // System state simulation
    bool audio_alert_active;
    bool haptic_alert_active;
    bool visual_alert_active;
    bool sos_button_pressed;
    uint32_t alert_start_time;

    // Test result tracking
    struct TestResult {
        bool test_passed;
        uint32_t detection_time_ms;
        uint8_t final_confidence_score;
        FallConfidence_t final_confidence_level;
        FallStatus_t final_fall_status;
        uint32_t total_sensor_samples;
        bool false_positive;
        bool false_negative;
    };

    // Helper functions
    void assert_true(bool condition, const char* test_name);
    void print_test_result(bool passed, const char* test_name);
    void simulateMainLoop(uint32_t duration_ms, TestResult& result);
    void processDetectionLogic(SensorData_t& data, TestResult& result);
    void updateAlertSystem();
    void printSystemStatus(const SensorData_t& data);

public:
    TestIntegration();
    ~TestIntegration();

    // Configuration
    void setTestConfig(const TestConfig& test_config);
    void setVerboseOutput(bool verbose);

    // Complete scenario tests (reproducing main.cpp behavior)
    bool testCompleteNormalActivity();
    bool testCompleteWalkingActivity();
    bool testCompleteTypicalFall();
    bool testCompleteSevereFall();
    bool testCompleteFalsePositiveDrop();
    bool testCompleteFalsePositiveExercise();

    // Edge case integration tests
    bool testSOSButtonDuringNormalActivity();
    bool testSOSButtonDuringFallDetection();
    bool testRecoveryAfterPartialDetection();
    bool testMultipleConsecutiveFalls();
    bool testSensorMalfunctionHandling();

    // Performance and stress tests
    bool testExtendedOperationStability();
    bool testHighFrequencySensorData();
    bool testMemoryLeakDetection();

    // System validation tests
    bool testAlertSystemActivation();
    bool testConfidenceScoreAccuracy();
    bool testDetectionTimingRequirements();

    // Comprehensive test scenarios
    bool testRealWorldScenarioSequence();
    bool testBorderlineCaseAnalysis();

    // Main test suite
    bool runAllIntegrationTests();
    void printDetailedTestSummary();

private:
    // Scenario-specific test helpers
    TestResult runScenarioTest(TestScenario_t scenario, uint32_t duration_ms,
                              const char* scenario_name, bool should_detect_fall);
    bool validateTestResult(const TestResult& result, bool should_detect_fall,
                           const char* test_name);
    void printScenarioResults(const TestResult& result, const char* scenario_name);
};

#endif // TEST_INTEGRATION_H