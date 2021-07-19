#pragma once

#include <cmath>

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

void displayLine(int band, int y, rgb24 color)
{
  int xStart = band * (kMatrixWidth / NR_OF_BARS);
  for (int x = 0; x < kMatrixWidth / NR_OF_BARS; x++)
  {
    backgroundLayer.drawPixel(xStart + x, y, color);
  }
}

void displayBand(int band, float value, float peak, int y0, bool invert)
{
  // color hue based on band
  rgb24 color = toRGB24(CRGB(CHSV((band * 255) / (NR_OF_BARS - 1), 255, 255)));
  int barHeight = (kMatrixHeight - 1) * value;
  barHeight = barHeight < 0 ? 0 : barHeight;
  barHeight = barHeight > (kMatrixHeight - 1) ? (kMatrixHeight - 1) : barHeight;
  for (int y = 0; y < barHeight; y++)
  {
      displayLine(band, invert ? y0 - y : y0 + y, color);
  }
  if (peak > (0.5f / kMatrixHeight))
  {
    rgb24 peakColor = toRGB24(CRGB(CHSV((band * 255) / (NR_OF_BARS - 1), 100, 200)));
    int peakY = (kMatrixHeight - 1) * peak;
    peakY = peakY < 0 ? 0 : peakY;
    peakY = peakY > (kMatrixHeight - 1) ? (kMatrixHeight - 1) : peakY;
    displayLine(band, invert ? y0 - peakY : y0 + peakY, peakColor);
  }
}

void drawSpectrumCentered()
{
    backgroundLayer.fillScreen(toRGB24(CRGB(0, 0, 0)));
    for (int i = 0; i < NR_OF_BARS; i++)
    {
      displayBand(i, levels[i], peaks[i], kMatrixHeight / 2, true);
      displayBand(i, levels[i], peaks[i], kMatrixHeight / 2, false);
    }
    backgroundLayer.swapBuffers();
}

float angleStart = 0.0F;

void drawSpectrumRays(bool rotate)
{
    backgroundLayer.fillScreen(toRGB24(CRGB(0, 0, 0)));
    const Point center = {kMatrixWidth / 2, kMatrixHeight / 2};
    constexpr float angleDelta = 2.0f * M_PI / NR_OF_BARS;
    angleStart += rotate ? 0.01F : 0;
    for (int i = 0; i < NR_OF_BARS; i++)
    {
        float angle0 = angleStart + static_cast<float>(i) * angleDelta;
        float angle1 = angle0 + angleDelta;
        float levelRadius = 1.5f * kMatrixHeight * levels[i];
        float peakRadius = 1.5f * kMatrixHeight * peaks[i];
        if (levelRadius > 0.5f)
        {
            rgb24 color = toRGB24(CRGB(CHSV((i * 255) / (NR_OF_BARS - 1), 255, 255)));
            auto p1 = polarToCartesian(levelRadius, angle0);
            auto p2 = polarToCartesian(levelRadius, angle1);
            backgroundLayer.fillTriangle(center.x, center.y, center.x + p1.x, center.y + p1.y, center.x + p2.x, center.y + p2.y, color);
        }
        if (peakRadius > 0.5f)
        {
          rgb24 color = toRGB24(CRGB(CHSV((i * 255) / (NR_OF_BARS - 1), 100, 200)));
          auto p1 = polarToCartesian(peakRadius, angle0);
          auto p2 = polarToCartesian(peakRadius, angle1);
          backgroundLayer.drawLine(center.x + p1.x, center.y + p1.y, center.x + p2.x, center.y + p2.y, color);
        }
    }
    backgroundLayer.swapBuffers();
}
