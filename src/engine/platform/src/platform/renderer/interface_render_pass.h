#pragma once
#include "../errors/renderer_errors.h"
#include <engine_types/handle/renderer/render_handle.h>
#include <engine_types/renderer/description/pipeline_description.h>
#include <engine_types/renderer/render_graph.h>
#include <foundation/algorithm/dependency_bounds.h>
#include <foundation/result/result.h>
#include <foundation/str/str.h>
#include <platform/renderer/interface_renderer.h>
#include <platform/renderer/updater/interface_resource_updater.h>
#include <span>

namespace enishi::platform {
    class IRenderPass {
      public:
        virtual ~IRenderPass(void) noexcept = default;

        [[nodiscard]] virtual std::span<const types::DrawCommand> get_commands(
            void) const noexcept = 0;

        [[nodiscard]] virtual types::RenderHandle get_render_target(void) const noexcept = 0;

        [[nodiscard]] virtual std::span<const types::RenderHandle> get_shader_reflections(
            void) const noexcept = 0;

        [[nodiscard]] virtual foundation::VoidResult<RenderError> make_from_description(
            const types::PipelineDescription& description,
            std::span<IRenderPass* const> dependency_render_passes,
            foundation::UTF8&& pass_name,
            foundation::DependencyNode&& node,
            foundation::DependencyBounds&& dependencies) noexcept = 0;

        virtual void add_shader(const types::RenderHandle& shader) noexcept = 0;

        virtual void add_mesh(const types::RenderHandle& mesh) noexcept = 0;

        virtual void add_updater(std::shared_ptr<IResourceUpdater> updater) noexcept = 0;

        virtual void update(void) = 0;

        [[nodiscard]] virtual foundation::UTF8 get_name(void) const noexcept = 0;

        [[nodiscard]] virtual foundation::DependencyNode get_node(void) const noexcept = 0;

        [[nodiscard]] virtual foundation::DependencyBounds get_dependencies(
            void) const noexcept = 0;
    };
} // namespace enishi::platform