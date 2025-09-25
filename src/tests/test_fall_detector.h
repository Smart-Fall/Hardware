#ifndef TEST_FALL_DETECTOR_H
#define TEST_FALL_DETECTOR_H

#include "../detection/fall_detector.h"
#include "../utils/data_types.h"
#include "fake_data_generator.h"
#include <Arduino.h>

class TestFallDetector {
private:
    FallDetector detector;
    FakeDataGenerator data_generator;
    int tests_passed;
    int tests_failed;

    void assert_equal(FallStatus_t expected, FallStatus_t actual, const char* test_name);
    void assert_true(bool condition, const char* test_name);
    void print_test_result(bool passed, const char* test_name);

    // Helper functions for testing
    SensorData_t createTestData(float ax, float ay, float az, float gx, float gy, float gz);
    void feedDataSequence(SensorData_t* data_array, int count, int delay_ms = 10);

public:
    TestFallDetector();
    ~TestFallDetector();

    // Basic functionality tests
    bool testInitialization();
    bool testResetDetection();
    bool testThresholdConfiguration();

    // Stage detection tests
    bool testStage1FreeFallDetection();
    bool testStage2ImpactDetection();
    bool testStage3RotationDetection();
    bool testStage4InactivityDetection();

    // Complete scenario tests
    bool testTypicalFallSequence();
    bool testSevereFallSequence();
    bool testFalsePositiveRejection();
    bool testTimeoutHandling();

    // Edge case tests
    bool testPartialFallSequence();
    bool testRecoveryDuringDetection();
    bool testSensorDataValidation();

    // Integration tests with fake data
    bool testWithFakeDataGenerator();

    // Performance tests
    bool testDetectionTiming();
    bool testMemoryUsage();

    // Complete test suite
    bool runAllTests();
    void printTestSummary();
};

#endif // TEST_FALL_DETECTOR_H