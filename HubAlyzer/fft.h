#pragma once

#define FFT_SPEED_OVER_PRECISION
#define FFT_SQRT_APPROXIMATION
#include "arduinoFFT.h" // Arduino FFT library

#include <cmath>
#include <functional>

// FFT transform wrapper
// SAMPLE_COUNT = Number of audio samples to use for FFT. This will again allocate the amount of 4-byte float values
// SAMPLE_RATE = Audio sample rate in Hz
template <unsigned SAMPLE_COUNT, unsigned SAMPLE_RATE_HZ = 48000>
class FFT
{
public:
    /// @brief Construct a new FFT transform
    /// @p amplitudes Amplitude values for individual frequency bands from the FFT. This MUST be allocated from the outside!
    FFT(float (*samples)[SAMPLE_COUNT])
        : m_real(reinterpret_cast<float *>(samples)), m_fft(ArduinoFFT<float>(m_real, m_imag, SAMPLE_COUNT, SAMPLE_RATE_HZ, m_weighingFactors))
    {
    }

    float *amplitudes()
    {
        return m_real;
    }

    /// @brief Call to update FFT data from samples
    void update()
    {
        // apply windowing and FFT
        memset(m_imag, 0, sizeof(m_imag));
        // m_fft.windowing(FFTWindow::Hamming, FFTDirection::Forward);
        m_fft.windowing(FFTWindow::Blackman_Harris, FFTDirection::Forward);
        m_fft.compute(FFTDirection::Forward);
        // kill the DC part in bin 0
        // m_real[0] = 0;
        // m_imag[0] = 0;
        // m_fft.dcRemoval();
        // calculate magnitude values from real + imaginary values
        m_fft.complexToMagnitude();
    }

private:
    float m_weighingFactors[SAMPLE_COUNT] = {0};
    float *m_real = nullptr;
    float m_imag[SAMPLE_COUNT] = {0};
    ArduinoFFT<float> m_fft;
};
