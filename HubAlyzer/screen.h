#pragma once

#include "color.h"

#include <SmartMatrix.h>

// Interface for abstract screens
class Screen
{
public:
    // Blit src buffer to screen back buffer
    virtual void blit(const RGBf *src) = 0;

    // Swap back buffer to display system
    virtual void swap() = 0;
};

// Screen implementation for SmartMatrix library screens
template <int WIDTH, int HEIGHT, unsigned OPTIONS>
class SMLayerScreen : public Screen
{
private:
    static constexpr int Width = WIDTH;
    static constexpr int Height = HEIGHT;
    static constexpr int MaxX = Width - 1;
    static constexpr int MaxY = Height - 1;

public:
    SMLayerScreen(SMLayerBackground<SM_RGB, OPTIONS> &layer)
        : m_layer(layer)
    {
    }

    virtual void blit(const RGBf *src) override
    {
        // Convert float buffer to rgb24
        auto dest = m_layer.backBuffer();
        for (unsigned i = 0; i < WIDTH * HEIGHT; i++)
        {
            dest[i].red = static_cast<uint8_t>(255.0F * src[i].r);
            dest[i].green = static_cast<uint8_t>(255.0F * src[i].g);
            dest[i].blue = static_cast<uint8_t>(255.0F * src[i].b);
        }
    }

    virtual void swap() override
    {
        m_layer.swapBuffers();
    }

private:
    SMLayerBackground<SM_RGB, OPTIONS> &m_layer{};
};
