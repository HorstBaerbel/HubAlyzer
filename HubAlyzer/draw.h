#pragma once

#include <SmartMatrix.h>
#include <FastLED.h>

#include <cmath>

template<int WIDTH, int HEIGHT, unsigned OPTIONS>
class Draw
{
private:
  static constexpr int Width = WIDTH;
  static constexpr int Height = HEIGHT;
  static constexpr int MaxX = Width - 1;
  static constexpr int MaxY = Height - 1;

  rgb24 toRGB24(const CRGB &c)
  {
    return rgb24{c.r, c.g, c.b};
  }

  struct Point
  {
    float x;
    float y;
  };

  // Convert polar coordinates (radius, angle) to cartesian (x, y)
  Point polarToCartesian(float radius, float angle)
  {
    //auto sincos = sincosf_fast(radius_angle.second);
    //return std::make_pair(radius_angle.first * sincos.second, radius_angle.first * sincos.first);
    return {radius * cosf(angle), radius * sinf(angle)};
  }

  template<int NR_OF_BANDS>
  void displayLine(int band, int y, rgb24 color)
  {
    int xStart = band * (Width / NR_OF_BANDS);
    for (int x = 0; x < Width / NR_OF_BANDS; x++)
    {
      m_layer.drawPixel(xStart + x, y, color);
    }
  }

  template<int NR_OF_BANDS>
  void displayBand(int band, float value, float peak, int y0, float scaleFactor, bool invert)
  {
    int x = band * (Width / NR_OF_BANDS);
    // color hue based on band
    rgb24 color = toRGB24(CRGB(CHSV((band * 255) / (NR_OF_BANDS - 1), 255, 255)));
    // draw bar until last pixel
    float barHeightf = MaxY * value * scaleFactor;
    int barHeight = trunc(barHeightf);
    barHeight = barHeight < 0 ? 0 : barHeight;
    barHeight = barHeight > MaxY ? MaxY : barHeight;
    for (int y = 0; y < barHeight - 1; y++)
    {
        m_layer.drawPixel(x, invert ? y0 - y : y0 + y, color);
    }
    // draw final pixel
    float barRest = barHeightf - barHeight;
    rgb24 restColor = toRGB24(CRGB(CHSV((band * 255) / (NR_OF_BANDS - 1), 255, 255 * barRest)));
    m_layer.drawPixel(x, invert ? y0 - (barHeight - 1) : y0 + (barHeight - 1), restColor);
    // draw peak
    if (peak > (0.5f / Height))
    {
      rgb24 peakColor = toRGB24(CRGB(CHSV((band * 255) / (NR_OF_BANDS - 1), 100, 150)));
      int peakY = MaxY * peak * scaleFactor;
      peakY = peakY < 0 ? 0 : peakY;
      peakY = peakY > MaxY ? MaxY : peakY;
      m_layer.drawPixel(x, invert ? y0 - peakY : y0 + peakY, peakColor);
    }
  }

public:
  Draw(SMLayerBackground<SM_RGB, OPTIONS> & layer)
    : m_layer(layer)
  {}

  template<int NR_OF_BANDS>
  void spectrumCentered(const float * levels, const float * peaks)
  {
      m_layer.fillScreen(toRGB24(CRGB(0, 0, 0)));
      for (int i = 0; i < NR_OF_BANDS; i++)
      {
        displayBand<NR_OF_BANDS>(i, levels[i], peaks[i], Height / 2, 0.5f, true);
        displayBand<NR_OF_BANDS>(i, levels[i], peaks[i], Height / 2, 0.5f, false);
      }
      m_layer.swapBuffers();
  }

  template<int NR_OF_BANDS>
  void spectrumRays(const float * levels, const float * peaks, bool rotate)
  {
      m_layer.fillScreen(toRGB24(CRGB(0, 0, 0)));
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
              rgb24 color = toRGB24(CRGB(CHSV((i * 255) / (NR_OF_BANDS - 1), 255, 255)));
              auto p1 = polarToCartesian(levelRadius, angle0);
              auto p2 = polarToCartesian(levelRadius, angle1);
              m_layer.fillTriangle(center.x, center.y, center.x + p1.x, center.y + p1.y, center.x + p2.x, center.y + p2.y, color);
          }
          if (peakRadius > 0.5f)
          {
            rgb24 color = toRGB24(CRGB(CHSV((i * 255) / (NR_OF_BANDS - 1), 100, 150)));
            auto p1 = polarToCartesian(peakRadius, angle0);
            auto p2 = polarToCartesian(peakRadius, angle1);
            m_layer.drawLine(center.x + p1.x, center.y + p1.y, center.x + p2.x, center.y + p2.y, color);
          }
      }
      m_layer.swapBuffers();
  }

private:
  SMLayerBackground<SM_RGB, OPTIONS> & m_layer{};
  float angleStart = 0.0F;
};
