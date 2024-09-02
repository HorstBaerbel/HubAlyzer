#pragma once

#include "color.h"

#include <SmartMatrix.h>
#include <cmath>

template <int WIDTH, int HEIGHT>
class Draw
{
private:
  static constexpr int Width = WIDTH;
  static constexpr int Height = HEIGHT;
  static constexpr int MaxX = Width - 1;
  static constexpr int MaxY = Height - 1;

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

  template <int NR_OF_BANDS>
  void displayLine(RGBf *dest, int band, int y, rgb24 color)
  {
    int xStart = band * (Width / NR_OF_BANDS);
    int index = y * WIDTH + xStart;
    for (int x = 0; x < Width / NR_OF_BANDS; x++)
    {
      dest[index + x] = color;
    }
  }

  template <int NR_OF_BANDS>
  void displayBand(RGBf *dest, int band, float value, float peak, int y0, float scaleFactor, bool invert)
  {
    int x = band * (Width / NR_OF_BANDS);
    // color hue based on band
    auto color = RGBf(HSVf((float)band / (NR_OF_BANDS - 1), 1.0F, 1.0F));
    // draw bar until last pixel
    float barHeightf = MaxY * value * scaleFactor;
    int barHeight = trunc(barHeightf);
    barHeight = barHeight < 0 ? 0 : barHeight;
    barHeight = barHeight > MaxY ? MaxY : barHeight;
    for (int y = 0; y < barHeight - 1; y++)
    {
      dest[(invert ? y0 - y : y0 + y) * WIDTH + x] = color;
    }
    // draw final pixel
    float barRest = barHeightf - barHeight;
    auto restColor = RGBf(HSVf((float)band / (NR_OF_BANDS - 1), 1.0F, barRest));
    dest[(invert ? y0 - (barHeight - 1) : y0 + (barHeight - 1)) * WIDTH + x] = restColor;
    // draw peak
    if (peak > (0.5f / Height))
    {
      auto peakColor = RGBf(HSVf((float)band / (NR_OF_BANDS - 1), 0.4F, 0.2F));
      int peakY = MaxY * peak * scaleFactor;
      peakY = peakY < 0 ? 0 : peakY;
      peakY = peakY > MaxY ? MaxY : peakY;
      dest[(invert ? y0 - peakY : y0 + peakY) * WIDTH + x] = peakColor;
    }
  }

public:
  using Type = std::function<void(RGBf *dest)>;

  void fill(RGBf *dest, RGBf color)
  {
    for (unsigned i = 0; i < WIDTH * HEIGHT; i++)
    {
      dest[i] = color;
    }
  }

  template <int NR_OF_BANDS>
  void spectrumCentered(RGBf *dest, const float *levels, const float *peaks, bool isBeat)
  {
    for (int i = 0; i < NR_OF_BANDS; i++)
    {
      displayBand<NR_OF_BANDS>(i, levels[i], peaks[i], Height / 2, 0.5f, true);
      displayBand<NR_OF_BANDS>(i, levels[i], peaks[i], Height / 2, 0.5f, false);
    }
    if (isBeat)
    {
      dest[0] = RGBf{1.0F, 1.0F, 1.0F};
    }
  }

  template <int NR_OF_BANDS>
  void spectrumRays(RGBf *dest, const float *levels, const float *peaks, bool rotate)
  {
    const Point center = {Width / 2, Height / 2};
    constexpr float angleDelta = 2.0f * M_PI / NR_OF_BANDS;
    angleStart += rotate ? 0.01F : 0;
    for (int i = 0; i < NR_OF_BANDS; i++)
    {
      float angle0 = angleStart + static_cast<float>(i) * angleDelta;
      float angle1 = angle0 + angleDelta;
      float levelRadius = 1.5f * Height * levels[i];
      float peakRadius = 1.5f * Height * peaks[i];
      if (levelRadius > 0.5f)
      {
        auto color = RGBf(HSVf((float)i / (NR_OF_BANDS - 1), 1.0F, 1.0F));
        auto p1 = polarToCartesian(levelRadius, angle0);
        auto p2 = polarToCartesian(levelRadius, angle1);
        m_layer.fillTriangle(center.x, center.y, center.x + p1.x, center.y + p1.y, center.x + p2.x, center.y + p2.y, color);
      }
      if (peakRadius > 0.5f)
      {
        auto color = RGBf(HSVf((float)i / (NR_OF_BANDS - 1), 0.4F, 0.2F));
        auto p1 = polarToCartesian(peakRadius, angle0);
        auto p2 = polarToCartesian(peakRadius, angle1);
        m_layer.drawLine(center.x + p1.x, center.y + p1.y, center.x + p2.x, center.y + p2.y, color);
      }
    }
  }

private:
  float angleStart = 0.0F;
};
