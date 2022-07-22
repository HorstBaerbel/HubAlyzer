#pragma once

#include <cmath>
#include <functional>

// Spectrum analyzer
// SAMPLE_COUNT = Number of audio samples to use for FFT. This will allocate twice the ammount of 4-byte float values
// AUDIO_NOISE_DB = Audio noise floor in dB
// AUDIO_MAX_DB = Max. audio signal in dB
// NR_OF_BANDS = Number of spectrum bands to generate
// SAMPLE_RATE = Audio sample rate in Hz
template <unsigned SAMPLE_COUNT, unsigned int AUDIO_NOISE_DB = 33, unsigned int AUDIO_MAX_DB = 120, unsigned int NR_OF_BANDS = 32, unsigned int MAX_HZ = 4000, unsigned SAMPLE_RATE_HZ = 48000>
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
  static constexpr float BIN_MIN_HZ = 1.0f / SAMPLE_COUNT * SAMPLE_RATE_HZ;                          // -> ~46Hz for 1024 samples, 48kHz sample rate
  static constexpr float BIN_LOG_START = 0.305F;                                                     // 10^0 = 1
  static constexpr float BIN_LOG_END = log10f(float(MAX_HZ * SAMPLE_COUNT) / float(SAMPLE_RATE_HZ)); // 10^2.23 = ~171) for 1024 samples, 48kHz sample rate
  static constexpr float BIN_LOG_RANGE = BIN_LOG_END - BIN_LOG_START;

  static constexpr float PeakDecayPerUpdate = (0.2f * SAMPLE_COUNT) / SAMPLE_RATE_HZ; // What amount the peaks decay per update call
  static constexpr float AgcSpeedFactor = 0.01f;                                      // The speed of the "Automatic Gain Control" mechanism [0,1]
  static constexpr float AgcKeepLevel = 10.0f;                                        // The maximum ammount the "automatic gain control" mechanism will remove from the signal

  /// @brief Construct a new spectrum analyzer
  /// @p amplitudes Amplitude values for individual frequency bands from the FFT
  /// @p amplitudeToDbFunc Function that converts audio amplitude values to dB values. This is audio input system dependent and thus has to come from outside
  Spectrum(float (*amplitudes)[SAMPLE_COUNT], std::function<float(float)> amplitudeToDb)
      : m_amplitudes(reinterpret_cast<float *>(amplitudes)), m_amplitudeToDb(amplitudeToDb)
  {
    // Serial.print("Log end: "); Serial.println(BIN_LOG_END);
    //  build bin info, spacing frequency bars evenly on the logarithmic x-axis
    unsigned int currentBin = 1;
    for (int i = 0; i < NR_OF_BANDS; i++)
    {
      auto &bar = m_bands[i];
      auto bandBinCount = trunc(powf(10, BIN_LOG_START + (BIN_LOG_RANGE * (i + 1)) / NR_OF_BANDS) - powf(10, BIN_LOG_START + (BIN_LOG_RANGE * i) / NR_OF_BANDS));
      bar.start = currentBin;
      bar.end = currentBin + bandBinCount;
      float x = ((static_cast<float>(i) / (NR_OF_BANDS - 1)) / NR_OF_BANDS);
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
  void update()
  {
    // calculate band levels
    float tempLevels[NR_OF_BANDS];
    for (int i = 0; i < NR_OF_BANDS; i++)
    {
      auto const &band = m_bands[i]; // the band we've currently processing
      tempLevels[i] = 0;             // accumulated levels
      for (unsigned int vi = band.start; vi <= band.end; vi++)
      {
        // Calculate dB values from amplitudes. This should give values between ~[AUDIO_NOISE_DB, AUDIO_MAX_DB]
        auto value = m_amplitudeToDb(m_amplitudes[vi]);
        // remove noise floor and clamp to 0
        value = value > AUDIO_NOISE_DB ? value - AUDIO_NOISE_DB : 0;
        value = value < 0 ? 0 : value;
        // accumulate levels
        tempLevels[i] += value;
      }
      // normalize level
      tempLevels[i] /= band.end - band.start + 1;
    }
    // get average and minimum of all levels
    float tempAvg = 0.0f;
    float tempMin = AUDIO_MAX_DB;
    for (int i = 0; i < NR_OF_BANDS; i++)
    {
      tempAvg += tempLevels[i];
      tempMin = tempLevels[i] < tempMin ? tempLevels[i] : tempMin;
    }
    tempAvg /= NR_OF_BANDS;
    // calculate new running average. we use an average of the minimum and average here,
    // as both alone won't give goode results
    auto levelFuzz = 0.5f * tempAvg + 0.5f * tempMin;
    m_levelsAvg = AgcSpeedFactor * levelFuzz + (1.0f - AgcSpeedFactor) * m_levelsAvg;
    // calculate amount of AGC
    const auto agcLevel = m_levelsAvg;
    const auto agcFactor = 0.033333f * m_levelsAvg + 1.0f;
    // apply running average and calculate current levels
    for (int i = 0; i < NR_OF_BANDS; i++)
    {
      // apply AGC and normalize to dB range
      tempLevels[i] = tempLevels[i] - agcLevel;
      tempLevels[i] = tempLevels[i] < 0 ? 0 : tempLevels[i];
      tempLevels[i] *= agcFactor * 1.0f / (AUDIO_MAX_DB - AUDIO_NOISE_DB);
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
    unsigned int start = 0; // first FFT bin for band. filled in setupBars()
    unsigned int end = 0;   // last FFT bin for band. filled in setupBars()
  };
  BandInfo m_bands[NR_OF_BANDS];

  std::function<float(float)> m_amplitudeToDb{};
  float *m_amplitudes = nullptr;
  float m_levels[NR_OF_BANDS] = {0};
  float m_peaks[NR_OF_BANDS] = {0};
  float m_levelsAvg = 0.0f; // running average level
};
