#pragma once
#include "../errors/renderer_errors.h"
#include <engine_types/handle/renderer/render_handle.h>
#include <engine_types/renderer/description/pipeline_description.h>
#include <engine_types/renderer/render_graph.h>
#include <foundation/result/result.h>
#include <span>

namespace enishi::platform {
    class IRenderPass {
      public:
        virtual ~IRenderPass(void) noexcept = default;

        [[nodiscard]] virtual std::span<const types::DrawCommand> get_commands(
            void) const noexcept = 0;

        [[nodiscard]] virtual types::RenderHandle get_render_target(void) const noexcept = 0;

        [[nodiscard]] virtual void update(void) = 0;
    };
} // namespace enishi::platform