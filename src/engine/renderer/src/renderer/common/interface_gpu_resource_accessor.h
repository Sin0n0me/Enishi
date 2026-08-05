#pragma once
#include <engine_types/renderer/command/draw_type.h>
#include <foundation/option/option.h>
#include <foundation/result/result.h>
#include <memory>
#include <platform/renderer/interface_image_view.h>
#include <vector>

namespace enishi::renderer {
    class GPUResourceAccessor {
      public:
        virtual ~GPUResourceAccessor(void) noexcept = default;

        [[nodiscard]] virtual foundation::Option<std::shared_ptr<platform::IRenderTargetView>>
        get_render_target(const types::HandleId handle) const = 0;
        [[nodiscard]] virtual const std::vector<std::shared_ptr<platform::IRenderTargetView>>&
        get_render_targets(void) const = 0;

        [[nodiscard]] virtual foundation::Option<const types::DrawArgs&> get_draw_args(
            const types::HandleId handle) const = 0;
        [[nodiscard]] virtual foundation::Option<types::DrawArgs&> get_draw_args(
            const types::HandleId handle) = 0;
    };
} // namespace enishi::renderer