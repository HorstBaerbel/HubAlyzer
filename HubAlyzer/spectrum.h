#pragma once

#include <cmath>
#include <functional>

// Spectrum analyzer
// SAMPLE_COUNT = Number of samples / amplitudes in buffer
// NR_OF_BANDS = Number of spectrum bands to generate
// MAX_HZ = Maximum / end of frequency spectrum
// SAMPLE_RATE = Audio sample rate in Hz
template <unsigned SAMPLE_COUNT, unsigned int NR_OF_BANDS = 32, unsigned int MAX_HZ = 4000, unsigned SAMPLE_RATE_HZ = 48000>
class Spectrum
{
public:
  // Split FFT results into spectrum / frequency bins logarithmicaly
  // See: https://dsp.stackexchange.com/questions/49436/scale-fft-frequency-range-for-a-bars-graph
  // -> Bin number k to bin frequency:
  // f = k / SAMPLE_COUNT * SAMPLE_RATE_HZ
  // -> Bin frequency to bin index k
  // k = f * SAMPLE_COUNT / SAMPLE_RATE_HZ
  static constexpr float MIN_HZ = 1.0f / SAMPLE_COUNT * SAMPLE_RATE_HZ;         // Minimum frequency ~94Hz for 512 samples, 48kHz sample rate
  static constexpr float BIN_START = 1;                                         // Bin #0 is crap / DC offset, so we don't use it
  static constexpr float BIN_SIZE_HZ = float(SAMPLE_RATE_HZ) / SAMPLE_COUNT;    // Size of each FFT bin in Hz, ~46Hz at 48kHz and 512 samples
  static constexpr float NR_OF_BINS = (MAX_HZ - MIN_HZ) / BIN_SIZE_HZ;          // # of bins needed to get to MAX_HZ, ~83 bins to 4KHz, at 48kHz and 512 samples
  static constexpr float BINS_PER_BAND = NR_OF_BINS / NR_OF_BANDS;              // # of bins needed for one band ~2.6 bins, for 32 bands up to 4kHz
  static constexpr unsigned int FRACT_BINS_PER_BAND = std::ceil(BINS_PER_BAND); // # of bins we need to touch to calculate a band

  static constexpr float PeakDecayPerUpdate = (0.2f * SAMPLE_COUNT) / SAMPLE_RATE_HZ; // What amount the peaks decay per update call

  /// @brief Get normalized level data. Read NR_OF_BANDS levels from this
  const float *levels() const
  {
    return m_levels;
  }

  /// @brief Get normalized peak data. Read NR_OF_BANDS peaks from this
  const float *peaks() const
  {
    return m_peaks;
  }

  /// @brief Call to update spectrum data
  /// @p magnitudes Magnitude values for individual frequency bands from the FFT. Must be in the range [0,1]!
  void update(const float *magnitudes)
  {
    // calculate band levels
    float tempLevels[NR_OF_BANDS] = {0};
    float binIndex = BIN_START;
    float binRest = 1.0F;
    for (int bandIndex = 0; bandIndex < NR_OF_BANDS; bandIndex++)
    {
      // accumulate bins for band
      unsigned int bandBinStart = static_cast<unsigned int>(binIndex);
      float bandRest = BINS_PER_BAND;
      float binFactor = binRest;
      for (unsigned int i = 0; i < FRACT_BINS_PER_BAND; i++)
      {
        tempLevels[bandIndex] += binFactor * magnitudes[bandBinStart + i];
        bandRest -= binFactor;
        binFactor = bandRest >= 1.0F ? 1.0F : bandRest;
      }
      binIndex += BINS_PER_BAND;
      binRest = 1.0F - binFactor;
      // average accumulated bins
      tempLevels[bandIndex] *= 1.0F / BINS_PER_BAND;
    }
    // update band levels
    for (int i = 0; i < NR_OF_BANDS; i++)
    {
      m_levels[i] = 0.25f * m_levels[i] + 0.75f * tempLevels[i];
      m_peaks[i] = m_levels[i] > m_peaks[i] ? m_levels[i] : (m_peaks[i] > 0 ? m_peaks[i] - PeakDecayPerUpdate : 0);
      // Serial.println(levels[i], 1);
    }
  }

private:
  float m_levels[NR_OF_BANDS] = {0};
  float m_peaks[NR_OF_BANDS] = {0};
};
