#pragma once
#include "crack_mask.h"
#include <optional>
namespace enishi::effect {
    class ICrackSurface {
      public:
        virtual ~ICrackSurface() noexcept = default;
        virtual foundation::Result<void, EffectError> open(
            const CrackSettings& settings, types::TextureData mask) = 0;
        virtual foundation::Result<void, EffectError> draw(
            float opacity, std::optional<CrackImpact> impact = std::nullopt) = 0;
        virtual void hide() noexcept = 0;
        virtual void close() noexcept = 0;
    };
} // namespace enishi::effect
