#pragma once

#include "serial_printf.h"

#include <cmath>
#include <functional>

template <typename T>
T clamp(T value, T minimum, T maximum)
{
    return std::min(std::max(value, minimum), maximum);
}

// Simple beat detector
// SAMPLE_COUNT = Number of samples / amplitudes in buffer
// MAX_HZ = Maximum / end of frequency spectrum for beat detection
// SAMPLE_RATE = Audio sample rate in Hz
template <unsigned SAMPLE_COUNT, unsigned int MAX_HZ = 4000, unsigned SAMPLE_RATE_HZ = 48000>
class BeatDetection
{
    static constexpr unsigned int NR_OF_BINS_USED = (MAX_HZ * SAMPLE_COUNT) / SAMPLE_RATE_HZ + 1; // Maximum used bins from magnitudes array
    static constexpr float MAX_BEATS_PER_MINUTE = 200.0F;
    static constexpr float MIN_DELAY_BETWEEN_BEATS = 60000.0F / MAX_BEATS_PER_MINUTE;
    static constexpr float AVG_BEAT_DURATION = 100.0F;         // Good value range is [50:150]
    static constexpr unsigned int BEAT_MAGNITUDE_SAMPLES = 10; // Good value range is [5:15]
    static constexpr unsigned int NR_OF_BEATS = 3;             // Number of seperate beats to analyze. #0 contains overall values, #1 and #2 are actual beat values
    static constexpr float BEAT_PROBABILITY_THRESHOLD = 0.5f;

    struct BeatInfo
    {
        unsigned int start = 0; // first FFT bin for beat
        unsigned int end = 0;   // last FFT bin for beat
        float current = 0;
        float runningTotal = 0;
        float runningAverage = 0;
        float variance = 0;
        float history[BEAT_MAGNITUDE_SAMPLES] = {0};
    };

public:
    /// @brief Construct a new beat detector
    BeatDetection()
    {
        // build beat bin info
        m_beats[0].start = 1;
        m_beats[0].end = NR_OF_BINS_USED - 1;
        m_beats[1].start = 1;
        m_beats[1].end = m_beats[1].start + 2;
        m_beats[2].start = 1;
        m_beats[2].end = m_beats[2].start + 4;
    }

    /// @brief Call to update beat data
    /// @p magnitudes Magnitude values for individual frequency bands from the FFT. Must be in the range [0,1]!
    void update(const float *magnitudes)
    {
        for (int i = 0; i < NR_OF_BEATS; i++)
        {
            auto &beat = m_beats[i];
            // calculate average magnitudes of beat bin
            beat.current = getAverageMagnitude(beat, magnitudes);
            // process beat bin history values
            updateHistoryValues(beat, m_magnitudeSampleIndex);
        }
        for (int i = 0; i < NR_OF_BEATS; i++)
        {
            const auto &beat = m_beats[i];
            // Serial_printf("%f, %f, %f | ", beat.current, beat.runningAverage, beat.variance);
        }
        // Serial.println("");
        //  prepare the magnitude sample index for the next update
        m_magnitudeSampleIndex++;
        if (m_magnitudeSampleIndex >= BEAT_MAGNITUDE_SAMPLES)
        {
            m_magnitudeSampleIndex = 0;
        }
        // calculate beat probability
        auto magnitudeChange = calculateMagnitudeChangeFactor();
        auto variance = calculateVarianceFactor();
        auto recency = calculateRecencyFactor();
        // Serial_printf("%f, %f, %f\n", magnitudeChange, variance, recency);
        auto beatProbability = magnitudeChange * variance * recency;
        if (beatProbability >= BEAT_PROBABILITY_THRESHOLD)
        {
            m_lastBeatTimestamp = millis();
        }
    }

    long timeSinceLastBeatMs() const
    {
        return millis() - m_lastBeatTimestamp;
    }

private:
    float getAverageMagnitude(const BeatInfo &beat, const float *magnitudes)
    {
        float total = 0.0F;
        for (unsigned int i = beat.start; i <= beat.end; i++)
        {
            total += magnitudes[i];
        }
        return total / (beat.end - beat.start + 1);
    }

    void updateHistoryValues(BeatInfo &beat, unsigned int sampleIndex)
    {
        beat.runningTotal -= beat.history[sampleIndex]; // subtract the oldest history value from the running total
        beat.runningTotal += beat.current;              // add the current value to the running total
        beat.history[sampleIndex] = beat.current;       // set the current value in the history
        beat.runningAverage = beat.runningTotal / BEAT_MAGNITUDE_SAMPLES;
        // update the variance of frequency magnitudes
        float squaredDiffSum = 0;
        for (int i = 0; i < BEAT_MAGNITUDE_SAMPLES; i++)
        {
            auto magnitudeDiff = beat.history[i] - beat.runningAverage;
            squaredDiffSum += magnitudeDiff * magnitudeDiff;
        }
        beat.variance = squaredDiffSum / BEAT_MAGNITUDE_SAMPLES;
    }

    // Will calculate a value in range [0,1] based on the magnitude changes of different frequency bands. Low values are indicating a low beat probability.
    float calculateMagnitudeChangeFactor()
    {
        // current overall magnitude is higher than the average, probably because the signal is mainly noise
        float aboveAverageOverallMagnitudeFactor = m_beats[0].current / m_beats[0].runningAverage;
        // Serial_printf("%f, ", aboveAverageOverallMagnitudeFactor);
        aboveAverageOverallMagnitudeFactor *= 10;
        aboveAverageOverallMagnitudeFactor = clamp(aboveAverageOverallMagnitudeFactor, 0.0F, 1.0F);
        // current magnitude is higher than the average, probably because there's a beat right now
        float aboveAverageFirstMagnitudeFactor = m_beats[1].current / m_beats[1].runningAverage;
        // Serial_printf("%f, ", aboveAverageFirstMagnitudeFactor);
        aboveAverageFirstMagnitudeFactor *= 1.5;
        aboveAverageFirstMagnitudeFactor = pow(aboveAverageFirstMagnitudeFactor, 3);
        aboveAverageFirstMagnitudeFactor /= 3;
        aboveAverageFirstMagnitudeFactor = clamp(aboveAverageFirstMagnitudeFactor, 0.0F, 1.0F);
        float aboveAverageSecondMagnitudeFactor = m_beats[2].current / m_beats[2].runningAverage;
        // Serial_printf("%f\n", aboveAverageSecondMagnitudeFactor);
        aboveAverageSecondMagnitudeFactor *= 10;
        aboveAverageSecondMagnitudeFactor = clamp(aboveAverageSecondMagnitudeFactor, 0.0F, 1.0F);
        // combine values into magnitude change
        float magnitudeChangeFactor = aboveAverageFirstMagnitudeFactor;
        if (magnitudeChangeFactor > 0.15)
        {
            magnitudeChangeFactor = std::max(aboveAverageFirstMagnitudeFactor, aboveAverageSecondMagnitudeFactor);
        }

        if (magnitudeChangeFactor < 0.5 && aboveAverageOverallMagnitudeFactor > 0.5)
        {
            // there's no bass related beat, but the overall magnitude changed significantly
            magnitudeChangeFactor = std::max(magnitudeChangeFactor, aboveAverageOverallMagnitudeFactor);
        }
        else
        {
            // this is here to avoid treating signal noise as beats
            // magnitudeChangeFactor *= 1 - aboveAverageOverallMagnitudeFactor;
        }
        return magnitudeChangeFactor;
    }

    // Will calculate a value in range [0,1] based on variance in the first and second* frequency band over time.
    // The variance will be high if the magnitude of bass frequencies changed in the last few milliseconds. Low values are indicating a low beat probability.
    float calculateVarianceFactor()
    {
        // a beat also requires a high variance in recent frequency magnitudes
        float firstVarianceFactor = ((m_beats[1].variance - 50.0F) / 20.0F) - 1.0F;
        firstVarianceFactor = clamp(firstVarianceFactor, 0.0F, 1.0F);
        float secondVarianceFactor = ((m_beats[2].variance - 50.0F) / 20.0F) - 1.0F;
        secondVarianceFactor = clamp(secondVarianceFactor, 0.0F, 1.0F);
        float varianceFactor = std::max(firstVarianceFactor, secondVarianceFactor);
        return varianceFactor;
    }

    // Will calculate a value in range [0,1] based on the recency of the last detected beat. Low values are indicating a low beat probability.
    float calculateRecencyFactor()
    {
        float recencyFactor = 1.0F;
        auto durationSinceLastBeat = millis() - m_lastBeatTimestamp;
        float referenceDuration = MIN_DELAY_BETWEEN_BEATS - AVG_BEAT_DURATION;
        recencyFactor = 1.0F - (referenceDuration / durationSinceLastBeat);
        recencyFactor = clamp(recencyFactor, 0.0F, 1.0F);
        return recencyFactor;
    }

    BeatInfo m_beats[NR_OF_BEATS]; // beat values. #0 contains overall values, #1 and #2 are actual beat values
    long m_lastBeatTimestamp = 0;
    std::function<float(float)> m_amplitudeToDb{};
    unsigned int m_magnitudeSampleIndex = 0; // which index of BeatInfo::magnitudes will be samples next
    float *m_amplitudes = nullptr;
};
