#pragma once
#include <cstdint>
#include <platform/renderer/interface_render_command_encoder.h>
#include <platform/renderer/interface_renderer.h>
#include <platform/renderer/render_backend_description.h>
#include <platform/window/interface_window.h>
namespace enishi::platform_impl {
    struct EffectSurfaceContext {
        std::shared_ptr<platform::IRenderer> renderer;
        std::shared_ptr<platform::IRenderCommandEncoder> encoder;
        const platform::IWindow& window;
        const platform::RenderBackendDescription& backend;
    };
    struct EffectSurfaceFrame {
        std::int32_t x;
        std::int32_t y;
        std::int32_t width;
        std::int32_t height;
        float desktop_span;
        bool all_displays;
    };
    class IEffectSurfaceContent {
      public:
        virtual ~IEffectSurfaceContent() noexcept;
        // Called once with the window hidden and its rendering context current.
        [[nodiscard]] virtual platform::RenderResult<void> initialize(
            const EffectSurfaceContext& context) = 0;
        // The caller makes the matching context current before each draw and before destruction.
        [[nodiscard]] virtual platform::RenderResult<void> draw(
            const EffectSurfaceFrame& frame) = 0;
    };
} // namespace enishi::platform_impl
