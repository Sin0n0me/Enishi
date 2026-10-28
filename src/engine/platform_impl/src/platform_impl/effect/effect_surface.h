#pragma once
#include "interface_effect_surface_content.h"
#include <effect/effect_target.h>
#include <effect/effect_types.h>
#include <functional>
#include <memory>
#include <vector>
namespace enishi::platform_impl {
    class EffectSurface {
      public:
        using ContentFactory = std::function<std::unique_ptr<IEffectSurfaceContent>()>;

      private:
        struct Overlay;
        std::vector<Overlay> overlays;
        float desktop_span = 1;

      public:
        EffectSurface();
        ~EffectSurface() noexcept;
        EffectSurface(const EffectSurface&) = delete;
        EffectSurface& operator=(const EffectSurface&) = delete;
        [[nodiscard]] foundation::Result<void, effect::EffectError> open(
            const ContentFactory& factory);
        [[nodiscard]] foundation::Result<void, effect::EffectError> draw(
            const effect::EffectTarget& target);
        [[nodiscard]] bool hide() noexcept;
        void close() noexcept;

      private:
        foundation::Result<void, effect::EffectError> add_display(
            std::uint32_t display, const ContentFactory& factory);
    };
} // namespace enishi::platform_impl
