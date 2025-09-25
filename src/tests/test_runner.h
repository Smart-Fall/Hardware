#ifndef TEST_RUNNER_H
#define TEST_RUNNER_H

#include "test_confidence_scorer.h"
#include "test_fall_detector.h"
#include "test_integration.h"
#include "fake_data_generator.h"
#include "../utils/test_logger.h"
#include <Arduino.h>

class TestRunner {
private:
    TestConfidenceScorer confidence_tests;
    TestFallDetector detector_tests;
    TestIntegration integration_tests;

    int total_test_suites;
    int passed_test_suites;
    int failed_test_suites;

    uint32_t test_start_time;
    uint32_t test_end_time;

public:
    TestRunner();
    ~TestRunner();

    // Individual test suite runners
    bool runConfidenceScorerTests();
    bool runFallDetectorTests();
    bool runIntegrationTests();

    // Comprehensive test execution
    bool runAllTests();
    bool runQuickTests();
    bool runExtendedTests();

    // Test configuration
    void setVerboseOutput(bool verbose);
    void setQuickTestMode(bool quick_mode);

    // Results and reporting
    void printOverallSummary();
    void printSystemInfo();
    void printTestConfiguration();

    // Logging functions
    void initializeLogging();
    void finalizeLogging();
    void exportLogsToSerial();
    void printLogSummary();

    // Utility functions
    bool validateTestEnvironment();
    void setupTestEnvironment();
    void cleanupTestEnvironment();

    // Performance monitoring
    void printMemoryUsage();
    void printExecutionTime();
};

// Test configuration structure for easy parameter passing
struct SmartFallTestConfig {
    bool verbose_output = false;
    bool quick_mode = false;
    bool run_extended_tests = true;
    bool run_integration_tests = true;
    bool run_performance_tests = true;
    uint32_t max_test_duration_ms = 300000;  // 5 minutes total
};

#endif // TEST_RUNNER_H