#pragma once

#include "serial_printf.h"

#include <cmath>
#include <functional>

// Simple beat detector
// SAMPLE_COUNT = Number of samples / amplitudes in buffer
// MAX_HZ = Maximum / end of frequency spectrum for beat detection
// SAMPLE_RATE = Audio sample rate in Hz
template <unsigned SAMPLE_COUNT, unsigned int MAX_HZ = 4000, unsigned SAMPLE_RATE_HZ = 48000, unsigned UPDATE_RATE_HZ = 60>
class BeatDetection
{
    static constexpr float MIN_HZ = 1.0f / SAMPLE_COUNT * SAMPLE_RATE_HZ;      // Minimum frequency ~94Hz for 512 samples, 48kHz sample rate
    static constexpr float BIN_START = 1;                                      // Bin #0 is crap / DC offset, so we don't use it
    static constexpr float BIN_SIZE_HZ = float(SAMPLE_RATE_HZ) / SAMPLE_COUNT; // Size of each FFT bin in Hz, ~46Hz at 48kHz and 512 samples
    static constexpr float NR_OF_BINS = (MAX_HZ - MIN_HZ) / BIN_SIZE_HZ;       // # of bins needed to get to MAX_HZ, ~83 bins to 4KHz, at 48kHz and 512 samples

    static constexpr float BEAT_PROBABILITY_THRESHOLD = 0.2f;
    static constexpr float MIN_BEAT_INTERVAL_MS = 1000.0F / (180.0F / UPDATE_RATE_HZ);

    static constexpr unsigned int NR_OF_IIR_COEFFICIENTS = 4;

    struct BandInfo
    {
        unsigned int start = 0;
        unsigned int end = 0;
        float y[NR_OF_IIR_COEFFICIENTS + 1] = {0}; // output samples
        float x[NR_OF_IIR_COEFFICIENTS + 1] = {0}; // input samples
    };

public:
    static constexpr unsigned int NR_OF_BANDS = 2; // # of beat bands analyzed

    /// @brief Construct a new beat detector
    BeatDetection()
    {
        // build beat bin info
        m_bands[0].start = BIN_START;
        m_bands[0].end = m_bands[0].start;
        m_bands[1].start = m_bands[0].end + 1;
        m_bands[1].end = m_bands[1].start;
    }

    /// @brief Call to update beat data
    /// @p magnitudes Magnitude values for individual frequency bands from the FFT. Must be in the range [0,1]!
    const float *update(const float *magnitudes)
    {
        // calculate band levels
        for (int bandIndex = 0; bandIndex < NR_OF_BANDS; bandIndex++)
        {
            // accumulate bins for band
            auto &band = m_bands[bandIndex];
            float tempLevel = 0;
            for (unsigned int i = band.start; i <= band.end; i++)
            {
                tempLevel += magnitudes[i];
            }
            // average accumulated bins
            tempLevel /= band.end - band.start + 1;
            m_probabilities[bandIndex] = beatFilter(band, tempLevel);
        }
        // calculate beat probability
        auto beatProbability = m_probabilities[0] + m_probabilities[1];
        // Serial_printf("%f\n", beatProbability);
        if (beatProbability >= BEAT_PROBABILITY_THRESHOLD && (millis() - m_lastBeatTimestamp) > MIN_BEAT_INTERVAL_MS)
        {
            m_lastBeatTimestamp = millis();
        }
        return m_probabilities;
    }

    long timeSinceLastBeatMs() const
    {
        return millis() - m_lastBeatTimestamp;
    }

private:
    // IIR filter function
    float beatFilter(BandInfo &band, float sample)
    {
        // 2nd order IIR bandpass from 1Hz->3Hz @ 60Hz sampling rate
        static constexpr float IIRCoefficientsA[NR_OF_IIR_COEFFICIENTS + 1] = {
            0.01011856610623769600,
            0.00000000000000000000,
            -0.02023713221247539300,
            0.00000000000000000000,
            0.01011856610623769600};

        static constexpr float IIRCoefficientsB[NR_OF_IIR_COEFFICIENTS + 1] = {
            1.00000000000000000000,
            -3.64454212364758230000,
            5.04211562372784350000,
            -3.14029368609864170000,
            0.74365519504831146000};
        // shift the old samples
        for (unsigned int n = NR_OF_IIR_COEFFICIENTS; n > 0; n--)
        {
            band.x[n] = band.x[n - 1];
            band.y[n] = band.y[n - 1];
        }
        // calculate the new output
        band.x[0] = sample;
        band.y[0] = IIRCoefficientsA[0] * band.x[0];
        for (unsigned int n = 1; n <= NR_OF_IIR_COEFFICIENTS; n++)
        {
            band.y[0] += IIRCoefficientsA[n] * band.x[n] - IIRCoefficientsB[n] * band.y[n];
        }
        return band.y[0];
    }

    BandInfo m_bands[NR_OF_BANDS];
    float m_probabilities[NR_OF_BANDS] = {0};
    long m_lastBeatTimestamp = 0;
};
