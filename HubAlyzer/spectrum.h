#pragma once

#define FFT_SPEED_OVER_PRECISION
#define FFT_SQRT_APPROXIMATION
#include "arduinoFFT.h" // Arduino FFT library

#include <cmath>
#include <functional>

// Spectrum analyzer
// SAMPLE_COUNT = Number of audio samples to use for FFT. This will allocate twice the ammount of 4-byte float values 
// AUDIO_NOISE_DB = Audio noise floor in dB
// AUDIO_MAX_DB = Max. audio signal in dB
// NR_OF_BANDS = Number of spectrum bands to generate
// SAMPLE_RATE = Audio sample rate in Hz
template <unsigned SAMPLE_COUNT, unsigned int AUDIO_NOISE_DB = 33, unsigned int AUDIO_MAX_DB = 120, unsigned int NR_OF_BANDS = 32, unsigned int MAX_HZ = 8000, unsigned SAMPLE_RATE_HZ = 48000>
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
  static constexpr float BIN_MIN_HZ = 1.0f / SAMPLE_COUNT * SAMPLE_RATE_HZ; // -> ~46Hz for 1024 samples, 48kHz sample rate
  static constexpr float BIN_LOG_START = 0.305F; // 10^0 = 1
  static constexpr float BIN_LOG_END = log10f(float(MAX_HZ * SAMPLE_COUNT) / float(SAMPLE_RATE_HZ)); // 10^2.23 = ~171) for 1024 samples, 48kHz sample rate
  static constexpr float BIN_LOG_RANGE = BIN_LOG_END - BIN_LOG_START;

  static constexpr float PeakDecayPerFrame = (0.2f * SAMPLE_COUNT) / SAMPLE_RATE_HZ;

  /// @brief amplitudeToDbFunc Function that converts audio amplitude values to dB values. This is system dependent and thus has to come from outside
  Spectrum(std::function<float(float)> amplitudeToDb)
    : m_amplitudeToDb(amplitudeToDb)
  {
    //Serial.print("Log end: "); Serial.println(BIN_LOG_END);
    // build bin info, spacing frequency bars evenly on the logarithmic x-axis
    unsigned int currentBin = 1;
    for (int i = 0; i < NR_OF_BANDS; i++)
    {
      auto &bar = m_bands[i];
      auto barBinCount = trunc(powf(10, BIN_LOG_START + (BIN_LOG_RANGE * (i + 1)) / NR_OF_BANDS) - powf(10, BIN_LOG_START + (BIN_LOG_RANGE * i) / NR_OF_BANDS));
      bar.start = currentBin;
      bar.end = currentBin + barBinCount;
      float x = ((static_cast<float>(i) / (NR_OF_BANDS - 1)) / NR_OF_BANDS);
      //bar.noiseLevel = std::max(0.0f, 0.3f - 8.0f * powf(x - 0.2f, 2));
      currentBin = bar.end + 1;
      //Serial.print(bar.start);
      //Serial.print(", ");
      //Serial.println(bar.end);
    }
  }

  /// @brief Get data storage for input audio values. Write SAMPLE_COUNT audio samples to this
  float * input()
  {
    return m_real;
  }

  /// @brief Get normalized level data. Read NR_OF_BANDS levels from this
  const float * levels() const
  {
    return m_levels;
  }

  /// @brief Get normalized peak data. Read NR_OF_BANDS peaks from this
  const float * peaks() const
  {
    return m_peaks;
  }

  /// @brief Call to update spectrum data
  void update()
  {
    // apply FFT
    memset(m_imag, 0, sizeof(m_imag));
    //m_fft.windowing(FFTWindow::Hamming, FFTDirection::Forward);
    m_fft.windowing(FFTWindow::Blackman_Harris, FFTDirection::Forward);
    m_fft.compute(FFTDirection::Forward);
    // kill the DC part in bin 0
    //m_imag[0] = 0;
    m_fft.complexToMagnitude();
    //m_fft.dcRemoval();
    // Calculate dB values from amplitudes. This should give values between ~[AUDIO_NOISE_DB, AUDIO_OVERLOAD_DB]
    for (unsigned int i = 0; i < SAMPLE_COUNT; i++)
    {
      m_real[i] = m_amplitudeToDb(m_real[i]);
    }
    // calculate bin levels
    float tempLevels[NR_OF_BANDS];
    for (int i = 0; i < NR_OF_BANDS; i++)
    {
      auto const &bar = m_bands[i]; // the bar we've currently processing
      float barBinCount = 0;     // number of valid bins found
      tempLevels[i] = 0;         // accumulated levels
      for (unsigned int vi = bar.start; vi <= bar.end; vi++)
      {
        // accumulate values
        float value = m_real[vi];
        // normalize and clamp
        if (value >= 2.0f * AUDIO_NOISE_DB)
        {
          value = (value - 2.0f * AUDIO_NOISE_DB) * (1.0f / AUDIO_MAX_DB);
          value = value < 0 ? 0 : value;
          tempLevels[i] += value;
          barBinCount++;
        }
      }
      barBinCount = barBinCount == 0 ? 1 : barBinCount;
      tempLevels[i] = tempLevels[i] / barBinCount;
      /*if (tempLevels[i] < bar.noiseLevel) {
              tempLevels[i] = 0;
          }*/
      m_levels[i] = 0.25f * m_levels[i] + 0.75f * tempLevels[i];
      m_peaks[i] = m_levels[i] > m_peaks[i] ? m_levels[i] : (m_peaks[i] > 0 ? m_peaks[i] - PeakDecayPerFrame : 0);
      //Serial.println(levels[i], 1);
    }
  }

private:
  struct BandInfo
  {
    unsigned int start; // first FFT bin for bar. filled in setupBars()
    unsigned int end; // last FFT bin for bar. filled in setupBars()
    float noiseLevel; // system-dependent noise level for bar
  };
  BandInfo m_bands[NR_OF_BANDS];

  std::function<float(float)> m_amplitudeToDb{};
  float m_weighingFactors[SAMPLE_COUNT] = {0};
  float m_real[SAMPLE_COUNT] = {0};
  float m_imag[SAMPLE_COUNT] = {0};
  ArduinoFFT<float> m_fft = ArduinoFFT<float>(m_real, m_imag, SAMPLE_COUNT, SAMPLE_RATE_HZ, m_weighingFactors);
  float m_levels[NR_OF_BANDS] = {0};
  float m_peaks[NR_OF_BANDS] = {0};
};
