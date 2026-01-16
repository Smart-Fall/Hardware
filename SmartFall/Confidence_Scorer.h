#ifndef CONFIDENCE_SCORER_H
#define CONFIDENCE_SCORER_H

#include "Data_Types.h"
#include "Config.h"
#include <Arduino.h>

class ConfidenceScorer
{
private:
    // Scoring components
    uint8_t stage1_score; // Free fall scoring (max 25 points)
    uint8_t stage2_score; // Impact scoring (max 25 points)
    uint8_t stage3_score; // Rotation scoring (max 20 points)
    uint8_t stage4_score; // Inactivity scoring (max 20 points)
    uint8_t filter_score; // False positive filters (max 15 points)

    // Detailed scoring breakdown
    struct
    {
        uint8_t duration_score;
        uint8_t magnitude_score;
    } stage1_breakdown;

    struct
    {
        uint8_t impact_magnitude_score;
        uint8_t timing_score;
        uint8_t fsr_validation_score;
    } stage2_breakdown;

    struct
    {
        uint8_t angular_velocity_score;
        uint8_t orientation_change_score;
    } stage3_breakdown;

    struct
    {
        uint8_t inactivity_duration_score;
        uint8_t stability_score;
    } stage4_breakdown;

    struct
    {
        uint8_t pressure_filter_score;
        uint8_t fsr_filter_score;
    } filter_breakdown;

    struct
    {
        uint8_t heart_rate_elevation_score;
        uint8_t spo2_validation_score;
    } physiological_breakdown;

    bool scoring_active;
    uint32_t scoring_start_time;

public:
    ConfidenceScorer();
    ~ConfidenceScorer();

    // Core scoring functions
    void resetScore();
    void startScoring();

    // Stage scoring functions
    void addStage1Score(float duration_ms, float min_magnitude_g);
    void addStage2Score(float impact_g, float timing_ms, bool fsr_detected = false);
    void addStage3Score(float angular_velocity_dps, float orientation_change_deg);
    void addStage4Score(float inactivity_duration_ms, bool stable);

    // Filter scoring functions
    void addPressureFilterScore(float altitude_change_m);
    void addFSRFilterScore(bool impact_detected, bool strap_secure);

    // Physiological validation (MAX30102 heart rate sensor)
    void addPhysiologicalScore(uint16_t current_heart_rate, uint8_t spo2,
                               uint16_t baseline_heart_rate = 60);

    // Results and classification
    uint8_t getTotalScore();
    FallConfidence_t getConfidenceLevel();
    uint8_t getStageScore(uint8_t stage_number);

    // Detailed breakdown
    void getScoreBreakdown(uint8_t &s1, uint8_t &s2, uint8_t &s3, uint8_t &s4, uint8_t &filters);
    bool isValidFallSequence();

    // Utility functions
    bool isScoringActive();
    uint32_t getScoringDuration();

    // Debug functions
    void printScoreBreakdown();
    void printDetailedAnalysis();
    const char *getConfidenceString(FallConfidence_t confidence);

private:
    // Internal scoring calculation functions
    uint8_t calculateDurationScore(float duration_ms);
    uint8_t calculateMagnitudeScore(float magnitude_g);
    uint8_t calculateImpactScore(float impact_g);
    uint8_t calculateTimingScore(float timing_ms);
    uint8_t calculateAngularScore(float angular_velocity_dps);
    uint8_t calculateInactivityScore(float duration_ms);
    uint8_t calculatePressureScore(float altitude_change_m);

    // Validation functions
    bool validateScoreRange(uint8_t score, uint8_t max_score);
    void capScore(uint8_t &score, uint8_t max_value);
    void updateFilterScore();
};

#endif // CONFIDENCE_SCORER_H