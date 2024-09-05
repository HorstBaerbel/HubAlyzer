#pragma once

#include "color.h"
#include "effect.h"

#include <cmath>

namespace Effects
{

  template <unsigned WIDTH, unsigned HEIGHT, unsigned NR_OF_BANDS>
  class DrawSpectrum : public Effect
  {
  public:
    enum class Mode {BandsCentered, RaysCentered};

  private:
    static constexpr int Width = static_cast<int>(WIDTH);
    static constexpr int Height = static_cast<int>(HEIGHT);
    static constexpr int MaxX = Width - 1;
    static constexpr int MaxY = Height - 1;
    static constexpr int NrOfBands = static_cast<int>(NR_OF_BANDS);

    struct Point
    {
      float x;
      float y;
    };

    // Convert polar coordinates (radius, angle) to cartesian (x, y)
    Point polarToCartesian(float radius, float angle)
    {
      // auto sincos = sincosf_fast(radius_angle.second);
      // return std::make_pair(radius_angle.first * sincos.second, radius_angle.first * sincos.first);
      return {radius * cosf(angle), radius * sinf(angle)};
    }

    void displayLine(RGBf *dest, int band, int y, rgb24 color)
    {
      int xStart = band * (Width / NrOfBands);
      int index = y * WIDTH + xStart;
      for (int x = 0; x < Width / NrOfBands; x++)
      {
        dest[index + x] = color;
      }
    }

    void displayBand(RGBf *dest, int band, float value, float peak, int y0, float scaleFactor, bool invert)
    {
      int x = band * (Width / NrOfBands);
      x = x < 0 ? 0 : x;
      x = x > MaxX ? MaxX : x;
      // color hue based on band
      auto color = RGBf(HSVf((float)band / (NrOfBands - 1), 1.0F, 1.0F));
      // draw bar until last pixel
      float barHeightf = MaxY * value * scaleFactor;
      int barHeight = trunc(barHeightf);
      if (invert)
      {
        // draw top-down
        auto yMin = (y0 - barHeight) < 0 ? 0 : (y0 - barHeight);
        for (int y = y0; y > yMin; y--)
        {
          dest[y * WIDTH + x] = color;
        }
        // draw final pixel
        if (yMin > 0)
        {
          float barRest = barHeightf - barHeight;
          auto restColor = RGBf(HSVf((float)band / (NrOfBands - 1), 1.0F, barRest));
          dest[yMin * WIDTH + x] = restColor;
        }
      }
      else
      {
        // draw bottom-up
        auto yMax = (y0 + barHeight) > MaxY ? MaxY : (y0 + barHeight);
        for (int y = y0; y < yMax; y++)
        {
          dest[y * WIDTH + x] = color;
        }
        // draw final pixel
        if (yMax < MaxY)
        {
          float barRest = barHeightf - barHeight;
          auto restColor = RGBf(HSVf((float)band / (NrOfBands - 1), 1.0F, barRest));
          dest[yMax * WIDTH + x] = restColor;
        }
      }
      // draw peak
      if (peak > (0.5f / Height))
      {
        auto peakColor = RGBf(HSVf((float)band / (NrOfBands - 1), 0.4F, 0.2F));
        int peakY = MaxY * peak * scaleFactor;
        if (invert)
        {
          auto peakMin = (y0 - peakY) < 0 ? 0 : (y0 - peakY);
          dest[peakMin * WIDTH + x] = peakColor;
        }
        else
        {
          auto peakMax = (y0 + peakY) > MaxY ? MaxY : (y0 + peakY);
          dest[peakMax * WIDTH + x] = peakColor;
        }
      }
    }

    void spectrumCentered(RGBf *dest, const float *levels, const float *peaks, bool isBeat)
    {
      for (int i = 0; i < NrOfBands; i++)
      {
        displayBand(dest, i, levels[i], peaks[i], Height / 2, 0.5f, true);
        displayBand(dest, i, levels[i], peaks[i], Height / 2, 0.5f, false);
      }
      if (isBeat)
      {
        dest[0] = RGBf{1.0F, 1.0F, 1.0F};
      }
    }

    void spectrumRays(RGBf *dest, const float *levels, const float *peaks, float angle)
    {
      const Point center = {Width / 2, Height / 2};
      constexpr float angleDelta = 2.0F * M_PI / NrOfBands;
      for (int i = 0; i < NrOfBands; i++)
      {
        float angle0 = angle + static_cast<float>(i) * angleDelta;
        float angle1 = angle0 + angleDelta;
        float levelRadius = 1.5f * Height * levels[i];
        float peakRadius = 1.5f * Height * peaks[i];
        if (levelRadius > 0.5f)
        {
          auto color = RGBf(HSVf((float)i / (NrOfBands - 1), 1.0F, 1.0F));
          auto p1 = polarToCartesian(levelRadius, angle0);
          auto p2 = polarToCartesian(levelRadius, angle1);
          //fillTriangle(center.x, center.y, center.x + p1.x, center.y + p1.y, center.x + p2.x, center.y + p2.y, color);
        }
        if (peakRadius > 0.5f)
        {
          auto color = RGBf(HSVf((float)i / (NrOfBands - 1), 0.4F, 0.2F));
          auto p1 = polarToCartesian(peakRadius, angle0);
          auto p2 = polarToCartesian(peakRadius, angle1);
          //drawLine(center.x + p1.x, center.y + p1.y, center.x + p2.x, center.y + p2.y, color);
        }
      }
    }

  public:
    virtual auto render(RGBf *dest, [[maybe_unused]] const RGBf *src, const float *levels, const float *peaks, bool isBeat) -> void override
    {
      switch (m_mode)
      {
      case Mode::RaysCentered:
        spectrumRays(dest, levels, peaks, m_angle);
        m_angle += m_rotate ? 0.01F : 0;
        break;
      default:
        spectrumCentered(dest, levels, peaks, isBeat);
      }
    }

  private:
    Mode m_mode = Mode::BandsCentered;
    float m_angle = 0.0F;
    bool m_rotate = true;
  };

}
