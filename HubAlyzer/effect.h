#pragma once

#include "color.h"
#include "vec.h"

#include <memory>

template <typename T>
auto inline clamp(T value, T minimum, T maximum) -> T
{
    return value < minimum ? minimum : (value > maximum ? maximum : value);
}

// Interface for all effects rendering to or manipulating frame buffers
class Effect
{
public:
    enum class Type
    {
      ToDestination,
      ToSource,
      DestinationToSource,
      SourceToDestination
    };

    // Shared effect object
    using SPtr = std::shared_ptr<Effect>;

    // Reimplement this in derived effect classes
    // Per default effects are applied to destination only
    virtual auto type() const -> Type
    {
      return Type::ToDestination;
    }

    // Reimplement this in derived effect classes
    virtual auto render(RGBf *dest, const RGBf *src, const float *levels, const float *peaks, bool isBeat) -> void = 0;
};

class NopEffect : public Effect
{
public:
    // The goggles, they do nothing...
    virtual auto render([[maybe_unused]] RGBf *dest, [[maybe_unused]] const RGBf *src, [[maybe_unused]] const float *levels, [[maybe_unused]] const float *peaks, [[maybe_unused]] bool isBeat) -> void override
    {
    }
};
