#pragma once

#include "color.h"
#include "vec.h"
#include "effect.h"

#include <cmath>

namespace Effects
{

    // Move screen from center an amount to the left or right
    template <unsigned WIDTH, unsigned HEIGHT>
    class MoveFromCenter : public Effect
    {
    public:
        virtual auto type() const -> Type override
        {
          return Effect::Type::SourceToDestination;
        }

        virtual auto render(RGBf *dest, const RGBf *src, [[maybe_unused]] const float *levels, [[maybe_unused]] const float *peaks, [[maybe_unused]] bool isBeat) -> void override
        {
            moveFromCenterVertical<WIDTH, HEIGHT>(dest, src, m_dist);
        }

    private:
        template <unsigned SRC_WIDTH, unsigned SRC_HEIGHT>
        auto moveFromCenterHorizontal(RGBf *dest, const RGBf *src, float dist) -> void
        {
            for (int32_t y = 0; y < HEIGHT; y++)
            {
                const auto ty = y * SRC_WIDTH;
                float u = 0;
                for (int32_t x = 0; x < WIDTH / 2; x++)
                {
                  float tx = std::fmod(u, SRC_WIDTH - 1);
                  *dest++ = src[static_cast<int>(ty + tx)];
                  u += m_dist;
                }
                u = WIDTH / 2;
                for (int32_t x = WIDTH / 2; x < WIDTH; x++)
                {
                  float tx = std::fmod(u, SRC_WIDTH - 1);
                  *dest++ = src[static_cast<int>(ty + tx)];
                  u += m_dist;
                }
            }
        }

        template <unsigned SRC_WIDTH, unsigned SRC_HEIGHT>
        auto moveFromCenterVertical(RGBf *dest, const RGBf *src, float dist) -> void
        {
            for (int32_t x = 0; x < WIDTH; x++)
            {
                auto dst = dest + x;
                float v = HEIGHT / 2 - 1;
                for (int32_t y = HEIGHT / 2 - 1; y > 0; y--)
                {
                  float ty = std::fmod(v, SRC_HEIGHT - 1);
                  *dst = src[static_cast<int>(ty * SRC_WIDTH)];
                  dst += SRC_WIDTH;
                  v -= m_dist;
                }
                v = HEIGHT / 2;
                for (int32_t y = HEIGHT / 2; y < HEIGHT; y++)
                {
                  float ty = std::fmod(v, SRC_HEIGHT - 1);
                  *dst = src[static_cast<int>(ty * SRC_WIDTH)];
                  dst += SRC_WIDTH;
                  v += m_dist;
                }
            }
        }

        float m_dist = 0.5F;
    };

    // Rotate + zoom + blit buffer
    template <unsigned WIDTH, unsigned HEIGHT>
    class RotoBlit : public Effect
    {
    public:
        virtual auto type() const -> Type override
        {
          return Effect::Type::SourceToDestination;
        }

        virtual auto render(RGBf *dest, const RGBf *src, [[maybe_unused]] const float *levels, [[maybe_unused]] const float *peaks, [[maybe_unused]] bool isBeat) -> void override
        {
            rotoBlit<WIDTH, HEIGHT, true>(dest, src, m_position, m_angle, m_scale);
        }

    private:
        // Rotate + zoom + blit screen
        // @param shift Shift in x and y. Must be in (0,1)
        // @param angle Rotation angle in radians. Must be in (0,2*PI)
        // @param zoom Zoom factor. Must be > 0
        template <unsigned SRC_WIDTH, unsigned SRC_HEIGHT, bool ADDITIVE = false>
        auto rotoBlit(RGBf *dest, const RGBf *src, const vec2f_t &position, float angle, float scale) -> void
        {
            float sa = std::sin(angle);
            float ca = std::cos(angle);
            float invScaleY = 1.0F / (static_cast<float>(HEIGHT) / static_cast<float>(WIDTH) * scale);
            float pa = ca / invScaleY;
            float pb = -sa / invScaleY;
            float pc = sa / invScaleY;
            float pd = ca / invScaleY;
            float pa0 = pa * (position.x - WIDTH / 2);
            float pb0 = pb * (position.y - HEIGHT / 2);
            float pc0 = pc * (position.x - WIDTH / 2);
            float pd0 = pd * (position.y - HEIGHT / 2);
            const float uY = pb - pa * WIDTH;
            const float vY = pd - pc * WIDTH;
            float u = pb0 + pa0;
            float v = pd0 + pc0;
            for (int32_t y = 0; y < HEIGHT; y++)
            {
                for (int32_t x = 0; x < WIDTH; x++)
                {
                    float tx = std::fmod(u, SRC_WIDTH - 1);
                    float ty = std::fmod(v, SRC_HEIGHT - 1);
                    if constexpr (ADDITIVE)
                    {
                        auto out = *dest;
                        const auto & in = src[static_cast<int>(ty * SRC_WIDTH + tx)];
                        out.r = clamp(in.r + out.r, 0.0F, 1.0f);
                        out.g = clamp(in.g + out.g, 0.0F, 1.0f);
                        out.b = clamp(in.b + out.b, 0.0F, 1.0f);
                        *dest++ = out;
                    }
                    else
                    {
                        *dest++ = src[static_cast<int>(ty * SRC_WIDTH + tx)];
                    }
                    u += pa;
                    v += pc;
                }
                u += uY;
                v += vY;
            }
        }

        vec2f_t m_position = {WIDTH / 2, HEIGHT / 2};
        float m_angle = 0;
        float m_scale = 1.0F;
    };

    // Fade screen to black or white. t must be in [-1,1]
    template <unsigned WIDTH, unsigned HEIGHT>
    class ChangeBrightness : public Effect
    {
    public:
        virtual auto render(RGBf *dest, const RGBf *src, [[maybe_unused]] const float *levels, [[maybe_unused]] const float *peaks, [[maybe_unused]] bool isBeat) -> void override
        {
            changeBrightness(dest, src, m_t);
        }

    private:
        auto changeBrightness(RGBf *dest, [[maybe_unused]] const RGBf *src, float t) -> void
        {
            for (unsigned i = 0; i < WIDTH * HEIGHT; i++)
            {
                // note that these input colors are not linear RGB. we should probably gamma-correct them
                auto color = dest[i];
                auto r = color.r + t * color.r;
                auto g = color.g + t * color.g;
                auto b = color.b + t * color.b;
                color.r = clamp(r, 0.0F, 1.0F);
                color.g = clamp(g, 0.0F, 1.0F);
                color.b = clamp(b, 0.0F, 1.0F);
                dest[i] = color;
            }
        }

        float m_t = 1.0F;
    };

    // Decrease/increase screen saturation. t must be in [-1,1]
    template <unsigned WIDTH, unsigned HEIGHT>
    class ChangeSaturation : public Effect
    {
    public:
        virtual auto render(RGBf *dest, const RGBf *src, [[maybe_unused]] const float *levels, [[maybe_unused]] const float *peaks, [[maybe_unused]] bool isBeat) -> void override
        {
            changeSaturation(dest, src, m_t);
        }

    private:
        auto changeSaturation(RGBf *dest, [[maybe_unused]] const RGBf *src, float t) -> void
        {
            for (unsigned i = 0; i < WIDTH * HEIGHT; i++)
            {
                // note that these input colors are not linear RGB. we should probably gamma-correct them
                auto color = dest[i];
                auto y = 0.2126F * color.r + 0.7152F * color.g + 0.0722F * color.b;
                auto r = color.r + t * (color.r - y);
                auto g = color.g + t * (color.g - y);
                auto b = color.b + t * (color.b - y);
                color.r = clamp(r, 0.0F, 1.0F);
                color.g = clamp(g, 0.0F, 1.0F);
                color.b = clamp(b, 0.0F, 1.0F);
                dest[i] = color;
            }
        }

        float m_t = 1.0F;
    };

}
