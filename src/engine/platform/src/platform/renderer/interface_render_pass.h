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

        [[nodiscard]] virtual foundation::VoidResult<RenderError> make_render_pass(
            const types::PipelineDescription& description) noexcept = 0;

        [[nodiscard]] virtual foundation::VoidResult<RenderError> set_topology(
            const types::PrimitiveTopology topology) noexcept = 0;

        [[nodiscard]] virtual foundation::VoidResult<RenderError> set_render_target(
            const types::RenderHandle handle) noexcept = 0;

        [[nodiscard]] virtual foundation::VoidResult<RenderError> set_rasterizer(
            const types::RenderHandle handle) noexcept = 0;

        [[nodiscard]] virtual foundation::VoidResult<RenderError> set_vertex_layout(
            const types::RenderHandle handle) noexcept = 0;

        [[nodiscard]] virtual foundation::VoidResult<RenderError> enable_uniform_camera(
            void) noexcept = 0;

        [[nodiscard]] virtual foundation::VoidResult<RenderError> disable_uniform_camera(
            void) noexcept = 0;

        [[nodiscard]] virtual foundation::VoidResult<RenderError> add_shader(
            const types::RenderHandle handle) noexcept = 0;

        [[nodiscard]] virtual foundation::VoidResult<RenderError> add_mesh(
            const types::RenderHandle handle,
            const std::vector<types::RenderHandle>& shaders) noexcept = 0;

        [[nodiscard]] virtual foundation::VoidResult<RenderError> remove(
            const types::RenderHandle handle) noexcept = 0;

        [[nodiscard]] virtual foundation::VoidResult<RenderError> remove_latest(
            const types::RenderHandle handle) noexcept = 0;

        [[nodiscard]] virtual foundation::VoidResult<RenderError> set_sub_command(
            const types::SubCommand sub_command, const types::RenderHandle handle) noexcept = 0;

        [[nodiscard]] virtual const types::RenderPass& get_render_pass(void) const noexcept = 0;
    };
} // namespace enishi::platform