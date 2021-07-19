#pragma once

#include <cmath>

#define FFT_SPEED_OVER_PRECISION
#define FFT_SQRT_APPROXIMATION
#include "arduinoFFT.h" // Arduino FFT library
#include "approx.h" // fast log10f and sincosf approximation

// Split FFT results into spectrum / frequency bins logarithmicaly
// See: https://dsp.stackexchange.com/questions/49436/scale-fft-frequency-range-for-a-bars-graph
// -> Bin number k to bin frequency:
// f = k / SAMPLE_COUNT * SAMPLE_RATE_HZ
// -> Bin frequency to bin index k
// k = f * SAMPLE_COUNT / SAMPLE_RATE_HZ
// Bin #0 is crap / DC offset, so we don't use it. Then we split the remaining bins logarithmicaly, 
// that is, the number of bins the bars use increases logarithmicaly.
#define BIN_MIN_HZ (1.0f / SAMPLE_COUNT * SAMPLE_RATE_HZ) // -> ~46Hz for 1024 samples, 48kHz sample rate
#define BIN_MAX_HZ 8000.0f
#define BIN_LOG_START 0.305F // 10^0 = 1
#define BIN_LOG_END log10f((BIN_MAX_HZ * SAMPLE_COUNT) / SAMPLE_RATE_HZ) // 10^2.23 = ~171) for 1024 samples, 48kHz sample rate
#define BIN_LOG_RANGE (BIN_LOG_END - BIN_LOG_START)

struct BarInfo
{
  unsigned int start; // first FFT bin for bar. filled in setupBars()
  unsigned int end; // last FFT bin for bar. filled in setupBars()
  float noiseLevel; // system-dependent noise level for bar
};
static const unsigned int NR_OF_BARS = 32;
BarInfo bars[NR_OF_BARS];

float weighingFactors[SAMPLE_COUNT] = {0};
float real[SAMPLE_COUNT] = {0};
float imag[SAMPLE_COUNT] = {0};
auto fft = ArduinoFFT<float>(real, imag, SAMPLE_COUNT, SAMPLE_RATE_HZ, weighingFactors);
float levels[NR_OF_BARS] = {0};
float peaks[NR_OF_BARS] = {0};
static constexpr float PeakDecayPerFrame = (0.2f * SAMPLE_COUNT) / SAMPLE_RATE_HZ;

void setupFFTBins()
{
  //Serial.print("Log end: "); Serial.println(BIN_LOG_END);
  // build bin info, spacing frequency bars evenly on the logarithmic x-axis
  unsigned int currentBin = 1;
  for (int i = 0; i < NR_OF_BARS; i++)
  {
    auto &bar = bars[i];
    auto barBinCount = trunc(powf(10, BIN_LOG_START + (BIN_LOG_RANGE * (i + 1)) / NR_OF_BARS) - powf(10, BIN_LOG_START + (BIN_LOG_RANGE * i) / NR_OF_BARS));
    bar.start = currentBin;
    bar.end = currentBin + barBinCount;
    float x = ((static_cast<float>(i) / (NR_OF_BARS - 1)) / NR_OF_BARS);
    //bar.noiseLevel = std::max(0.0f, 0.3f - 8.0f * powf(x - 0.2f, 2));
    currentBin = bar.end + 1;
    //Serial.print(bar.start);
    //Serial.print(", ");
    //Serial.println(bar.end);
  }
}
