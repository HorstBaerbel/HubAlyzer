#pragma once

#include <cmath>
#include <functional>

// Audio amplitude normalizer and automatic gain control
// SAMPLE_COUNT = Number of samples / amplitudes in buffer
// AUDIO_NOISE_DB = Audio noise floor in dB
// AUDIO_MAX_DB = Max. audio signal in dB
// MAX_HZ = Maximum / end of frequency spectrum
// SAMPLE_RATE = Audio sample rate in Hz
template <unsigned SAMPLE_COUNT, unsigned int AUDIO_NOISE_DB = 33, unsigned int AUDIO_MAX_DB = 120, unsigned int MAX_HZ = 4000, unsigned SAMPLE_RATE_HZ = 48000>
class Normalization
{
  static constexpr float MIN_HZ = 1.0f / SAMPLE_COUNT * SAMPLE_RATE_HZ;                                            // Minimum frequency ~94Hz for 512 samples, 48kHz sample rate
  static constexpr float BIN_START = 1;                                                                            // Bin #0 is crap / DC offset, so we don't use it
  static constexpr float BIN_SIZE_HZ = float(SAMPLE_RATE_HZ) / SAMPLE_COUNT;                                       // Size of each FFT bin in Hz, ~46Hz at 48kHz and 512 samples
  static constexpr unsigned int BINS_FOR_MAX_HZ = std::ceil((MAX_HZ - MIN_HZ) / BIN_SIZE_HZ) + BIN_START;          // # of bins needed to get to MAX_HZ, ~83 bins to 4KHz, at 48kHz and 512 samples
  static constexpr unsigned int NR_OF_BINS_USED = SAMPLE_COUNT < BINS_FOR_MAX_HZ ? SAMPLE_COUNT : BINS_FOR_MAX_HZ; // Maximum used bins from magnitudes array

  static constexpr float AgcSpeedFactor = 0.01f; // The speed of the "Automatic Gain Control" mechanism [0,1]
  static constexpr float AgcKeepLevel = 10.0f;   // The maximum amount the "automatic gain control" mechanism will remove from the signal

public:
  /// @brief Construct a new normalizer
  /// @p amplitudeToDbFunc Function that converts audio amplitude values to dB values. This is audio input system dependent and thus has to come from outside
  Normalization(std::function<float(float)> amplitudeToDb)
      : m_amplitudeToDb(amplitudeToDb)
  {
  }

  /// @brief Normalize amplitude values from [AUDIO_NOISE_DB, AUDIO_MAX_DB] to range [0,1] and apply gain control
  /// @p amplitudes Amplitude values for individual frequency bands from the FFT. Will be modified!
  /// @p applyAGC If true an automatic gain control will be applied to the amplitudes
  /// @p clearBin0 If true DC bin #0 will be set to 0
  /// @return Returns @p amplitudes converted to magnitudes in range [0,1] (where 0 is AUDIO_NOISE_DB and 1 is AUDIO_MAX_DB)
  float *apply(float *amplitudes, bool applyAGC = true, bool clearBin0 = true)
  {
    if (clearBin0)
    {
      amplitudes[0] = 0.0F;
    }
    // calculate bin levels
    for (unsigned int i = 0; i < NR_OF_BINS_USED; i++)
    {
      // Calculate dB values from amplitudes. This should give values between ~[AUDIO_NOISE_DB, AUDIO_MAX_DB]
      auto value = m_amplitudeToDb(amplitudes[i]);
      // remove noise floor and clamp to 0
      value -= 1.05F * AUDIO_NOISE_DB;
      amplitudes[i] = value < 0 ? 0 : value;
    }
    // check if we want to apply the AGC
    if (applyAGC)
    {
      // get average and minimum of all bins except #0
      float tempAvg = 0.0f;
      float tempMin = AUDIO_MAX_DB;
      for (unsigned int i = BIN_START; i < NR_OF_BINS_USED; i++)
      {
        tempAvg += amplitudes[i];
        tempMin = amplitudes[i] < tempMin ? amplitudes[i] : tempMin;
      }
      tempAvg *= 1.0F / NR_OF_BINS_USED;
      // calculate new running average. we use an average of the minimum and average here,
      // as both alone won't give goode results
      auto levelFuzz = 0.5f * tempAvg + 0.5f * tempMin;
      m_levelsAvg = AgcSpeedFactor * levelFuzz + (1.0f - AgcSpeedFactor) * m_levelsAvg;
      // calculate amount of AGC
      const auto agcLevel = m_levelsAvg;
      const auto agcFactor = 0.033333f * m_levelsAvg + 1.0f;
      // apply AGC and normalize to [0,1] range
      for (unsigned int i = 0; i < NR_OF_BINS_USED; i++)
      {
        amplitudes[i] = amplitudes[i] - agcLevel;
        amplitudes[i] = amplitudes[i] < 0 ? 0 : amplitudes[i];
        amplitudes[i] *= agcFactor * 1.0f / (AUDIO_MAX_DB - AUDIO_NOISE_DB);
      }
    }
    else
    {
      // normalize to [0,1] range
      for (unsigned int i = 0; i < NR_OF_BINS_USED; i++)
      {
        amplitudes[i] *= 1.0f / (AUDIO_MAX_DB - AUDIO_NOISE_DB);
      }
    }
    return amplitudes;
  }

private:
  std::function<float(float)> m_amplitudeToDb{};
  float m_levelsAvg = 0.0f; // running average level
};
