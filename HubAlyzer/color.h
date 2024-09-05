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
    auto hf = hsv.h * 6.0F;
    const auto hi = static_cast<int>(hf);
    auto f = hf - hi;
    auto p = hsv.v * (1.0F - hsv.s);
    auto q = hsv.v * (1.0F - hsv.s * f);
    auto t = hsv.v * (1.0F - hsv.s * (1.0F - f));
    switch (hi)
    {
    case 1:
        return RGBf(q, hsv.v, p);
    case 2:
        return RGBf(p, hsv.v, t);
    case 3:
        return RGBf(p, q, hsv.v);
    case 4:
        return RGBf(t, p, hsv.v);
    case 5:
        return RGBf(hsv.v, p, q);
    default:
        return RGBf(hsv.v, t, p);
    }
}

HSVf HSVf::fromRGB(const RGBf &rgb)
{
    HSVf hsv;
    auto cMax = std::max(std::max(rgb.r, rgb.g), rgb.b);
    auto cMin = std::min(std::min(rgb.r, rgb.g), rgb.b);
    auto cDelta = cMax - cMin;
    if (cDelta < 0.0001F)
    {
        hsv.h = 0.0F;
    }
    else if (cMax == rgb.r)
    {
        hsv.h = (60.0F / 360.0F) * ((rgb.g - rgb.b) / cDelta + 0.0F);
    }
    else if (cMax == rgb.g)
    {
        hsv.h = (60.0F / 360.0F) * ((rgb.b - rgb.r) / cDelta + 2.0F);
    }
    else if (cMax == rgb.b)
    {
        hsv.h = (60.0F / 360.0F) * ((rgb.r - rgb.g) / cDelta + 4.0F);
    }
    hsv.h = hsv.h < 0.0F ? hsv.h + 1.0F : hsv.h;
    hsv.s = cMax == 0 ? 0 : cDelta / cMax;
    hsv.v = cMax;
    return hsv;
}

RGBf lerp(const RGBf & a, const RGBf & b, float t)
{
    RGBf result;
    result.r = a.r + t * (b.r - a.r);
    result.g = a.g + t * (b.g - a.g);
    result.b = a.b + t * (b.b - a.b);
    return result;
}
