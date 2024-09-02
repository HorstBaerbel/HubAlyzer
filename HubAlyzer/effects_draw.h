#pragma once

#include "color.h"
#include "effect.h"

#include <cmath>

namespace Effects
{

  template <int WIDTH, int HEIGHT>
  class FillColor : public Effect
  {
  public:
    FillColor(const RGBf& color = {0, 0, 0})
      : m_color(color)
    {}


    virtual auto render(RGBf *dest, [[maybe_unused]] const RGBf *src, [[maybe_unused]] const float *levels, [[maybe_unused]] const float *peaks, [[maybe_unused]] bool isBeat) -> void override
    {
      fill(dest, m_color);
    }

  private:
    void fill(RGBf *dest, RGBf color)
    {
      for (unsigned i = 0; i < WIDTH * HEIGHT; i++)
      {
        dest[i] = color;
      }
    }

    RGBf m_color = {0, 0, 0};
  };

}
