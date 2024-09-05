#pragma once

#include "color.h"
#include "effect.h"

template <unsigned WIDTH, unsigned HEIGHT>
class EffectPipeline
{
public:
    EffectPipeline(std::vector<Effect::SPtr> &effects)
        : m_effects(effects)
    {
    }

    auto render(const float *levels, const float *peaks, bool isBeat) -> void
    {
        // swap buffers so output of previous frame is input for this frame
        std::swap(m_outBuffer, m_inBuffer);
        // loop through all effects
        for (auto &effect : m_effects)
        {
          switch(effect->type()) {
              case Effect::Type::ToDestination:
                  effect->render(m_outBuffer, nullptr, levels, peaks, isBeat);
                  break;
              case Effect::Type::ToSource:
                  effect->render(m_inBuffer, nullptr, levels, peaks, isBeat);
                  break;
              case Effect::Type::DestinationToSource:
                  effect->render(m_inBuffer, m_outBuffer, levels, peaks, isBeat);
                  break;
            default:
                  effect->render(m_outBuffer, m_inBuffer, levels, peaks, isBeat);
          }
        }
    }

    auto add(Effect::SPtr effect) -> void
    {
        m_effects.push_back(effect);
    }

    auto output() const -> const RGBf *
    {
        return m_outBuffer;
    }

private:
    std::vector<Effect::SPtr> m_effects;

    RGBf m_bufferA[WIDTH * HEIGHT];
    RGBf m_bufferB[WIDTH * HEIGHT];
    RGBf *m_inBuffer = m_bufferB;
    RGBf *m_outBuffer = m_bufferA;
};
