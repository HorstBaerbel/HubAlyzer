#pragma once

#include <cmath>

struct RGBf;
struct HSVf;

struct RGBf
{
    float r; // Red in range [0,1]
    float g; // Green in range [0,1]
    float b; // Blue in range [0,1]

    RGBf() = default;

    RGBf(float red, float green, float blue)
        : r(red), g(green), b(blue)
    {
    }

    RGBf(const HSVf &hsv)
    {
        *this = hsv;
    }

    RGBf &operator=(const HSVf &hsv)
    {
        *this = fromHSV(hsv);
        return *this;
    }

    static RGBf fromHSV(const HSVf &hsv);
};

struct HSVf
{
    float h; // Hue in range [0,1]
    float s; // Saturation in range [0,1]
    float v; // Value in range [0,1]

    HSVf() = default;

    HSVf(float hue, float saturation, float value)
        : h(hue), s(saturation), v(value)
    {
    }

    HSVf(const RGBf &rgb)
    {
        *this = rgb;
    }

    HSVf &operator=(const RGBf &rgb)
    {
        *this = fromRGB(rgb);
        return *this;
    }

    static HSVf fromRGB(const RGBf &rgb);
};

RGBf RGBf::fromHSV(const HSVf &hsv)
{
    RGBf rgb;
    auto c = hsv.v * hsv.s;
    auto x = c * (1.0F - std::abs(std::fmod(6.0F * hsv.h, 2.0F) - 1.0F));
    auto m = hsv.v - c;
    if (hsv.h < (60.0F / 360.0F))
    {
        rgb = RGBf(c, x, 0.0F);
    }
    else if (hsv.h < (120.0F / 360.0F))
    {
        rgb = RGBf(x, c, 0.0F);
    }
    else if (hsv.h < (180.0F / 360.0F))
    {
        rgb = RGBf(0.0F, c, x);
    }
    else if (hsv.h < (240.0F / 360.0F))
    {
        rgb = RGBf(0.0F, x, c);
    }
    else if (hsv.h < (300.0F / 360.0F))
    {
        rgb = RGBf(x, 0.0F, c);
    }
    else
    {
        rgb = RGBf(c, 0.0F, x);
    }
    rgb.r += m;
    rgb.g += m;
    rgb.b += m;
    return rgb;
}

HSVf HSVf::fromRGB(const RGBf &rgb)
{
    HSVf hsv;
    auto cMax = std::max(std::max(rgb.r, rgb.g), rgb.b);
    auto cMin = std::min(std::min(rgb.r, rgb.g), rgb.b);
    auto cDelta = cMax - cMin;
    if (cDelta == 0)
    {
        hsv.h = 0;
    }
    else if (cMax == rgb.r)
    {
        hsv.h = (60.0F / 360.0F) * std::fmod((rgb.g - rgb.b) / cDelta, 6.0F);
    }
    else if (cMax == rgb.g)
    {
        hsv.h = (60.0F / 360.0F) * ((rgb.b - rgb.r) / cDelta + 2.0F);
    }
    else if (cMax == rgb.b)
    {
        hsv.h = (60.0F / 360.0F) * ((rgb.r - rgb.g) / cDelta + 4.0F);
    }
    hsv.s = cMax == 0 ? 0 : cDelta / cMax;
    hsv.v = cMax;
    return hsv;
}
