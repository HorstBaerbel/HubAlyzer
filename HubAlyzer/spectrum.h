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
  // Bin #0 is crap / DC offset, so we don't use it. Then we split the remaining bins logarithmicaly,
  // that is, the number of bins the bars use increases logarithmicaly.
  static constexpr float BIN_MIN_HZ = 1.0f / SAMPLE_COUNT * SAMPLE_RATE_HZ;                          // -> ~94Hz for 512 samples, 48kHz sample rate
  static constexpr float BIN_LOG_START = 0;                                                          // 10^0 = 1, 10^0.305 = ~2
  static constexpr float BIN_LOG_END = log10f(float(MAX_HZ * SAMPLE_COUNT) / float(SAMPLE_RATE_HZ)); // 10^2.23 = ~171) for 1024 samples, 48kHz sample rate
  static constexpr float BIN_LOG_RANGE = BIN_LOG_END - BIN_LOG_START;

  static constexpr float PeakDecayPerUpdate = (0.2f * SAMPLE_COUNT) / SAMPLE_RATE_HZ; // What amount the peaks decay per update call

  /// @brief Construct a new spectrum analyzer
  Spectrum()
  {
    // Serial.print("Log end: "); Serial.println(BIN_LOG_END);
    //  build bin info, spacing frequency bars evenly on the logarithmic x-axis
    unsigned int currentBin = 1;
    for (int i = 0; i < NR_OF_BANDS; i++)
    {
      auto &bar = m_bands[i];
      auto startBin = powf(10, BIN_LOG_START + (BIN_LOG_RANGE * i) / NR_OF_BANDS);
      auto endBin = powf(10, BIN_LOG_START + (BIN_LOG_RANGE * (i + 1)) / NR_OF_BANDS);
      auto bandBinCount = trunc(endBin - startBin);
      bar.start = currentBin;
      bar.end = currentBin + bandBinCount;
      // float x = ((static_cast<float>(i) / (NR_OF_BANDS - 1)) / NR_OF_BANDS);
      // bar.noiseLevel = std::max(0.0f, 0.3f - 8.0f * powf(x - 0.2f, 2));
      currentBin = bar.end + 1;
      // Serial.print(bar.start);
      // Serial.print(", ");
      // Serial.println(bar.end);
    }
  }

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
    Serial.println(m_bands[NR_OF_BANDS - 1].end);
    // calculate band levels
    float tempLevels[NR_OF_BANDS] = {0};
    for (int bi = 0; bi < NR_OF_BANDS; bi++)
    {
      auto const &band = m_bands[bi];
      // accumulate levels
      for (unsigned int mi = band.start; mi <= band.end; mi++)
      {
        tempLevels[bi] += magnitudes[mi];
      }
      // average level
      tempLevels[bi] /= band.end - band.start + 1;
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
  struct BandInfo
  {
    unsigned int start = 0; // first FFT bin for band
    unsigned int end = 0;   // last FFT bin for band
  };
  BandInfo m_bands[NR_OF_BANDS];

  float m_levels[NR_OF_BANDS] = {0};
  float m_peaks[NR_OF_BANDS] = {0};
};
