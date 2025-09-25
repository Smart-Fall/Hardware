#ifndef TEST_CONFIDENCE_SCORER_H
#define TEST_CONFIDENCE_SCORER_H

#include "../detection/confidence_scorer.h"
#include "../utils/data_types.h"
#include <Arduino.h>

class TestConfidenceScorer {
private:
    ConfidenceScorer scorer;
    int tests_passed;
    int tests_failed;

    void assert_equal(uint8_t expected, uint8_t actual, const char* test_name);
    void assert_equal(FallConfidence_t expected, FallConfidence_t actual, const char* test_name);
    void assert_true(bool condition, const char* test_name);
    void print_test_result(bool passed, const char* test_name);

public:
    TestConfidenceScorer();
    ~TestConfidenceScorer();

    // Individual component tests
    bool testScoreReset();
    bool testStage1Scoring();
    bool testStage2Scoring();
    bool testStage3Scoring();
    bool testStage4Scoring();
    bool testFilterScoring();

    // Integration scoring tests
    bool testTypicalFallScoring();
    bool testSevereFallScoring();
    bool testFalsePositiveScoring();
    bool testBorderlineScoring();

    // Edge case tests
    bool testMaximumScore();
    bool testMinimumScore();
    bool testScoreValidation();

    // Complete test suite
    bool runAllTests();
    void printTestSummary();
};

#endif // TEST_CONFIDENCE_SCORER_H