#pragma once
#include <foundation/option/option.h>
#include <foundation/str/str.h>
#include <memory>
#include <platform/renderer/interface_render_pass.h>
#include <platform/renderer/updater/interface_resource_updater.h>
#include <platform/renderer/updater/interface_uniform_updater.h>
#include <unordered_map>

namespace enishi::render_pass {
    class RenderPass : public platform::IRenderPass {
      private:
        foundation::DependencyNode node;
        foundation::DependencyBounds dependencies;
        foundation::UTF8 name;
        types::RenderHandle render_target;
        std::vector<types::RenderHandle> shader_reflections;
        std::vector<types::DrawCommand> commands;
        std::unordered_map<foundation::UTF8, std::size_t> mesh_name_to_index;
        std::vector<std::shared_ptr<platform::IResourceUpdater>> resource_updater;

      public:
        RenderPass(void) = default;

        foundation::VoidResult<platform::RenderError> make_from_description(
            const types::PipelineDescription& description,
            std::span<platform::IRenderPass* const> dependency_render_passes,
            foundation::UTF8&& pass_name,
            foundation::DependencyNode&& node,
            foundation::DependencyBounds&& dependencies) noexcept override;

        void add_mesh(const foundation::UTF8& mesh_name, const types::RenderHandle handle);

      public:
        std::span<const types::DrawCommand> get_commands(void) const noexcept override;
        types::RenderHandle get_render_target(void) const noexcept override;
        std::span<const types::RenderHandle> get_shader_reflections(void) const noexcept override;
        void update(void) override;

        void add_updater(std::shared_ptr<platform::IResourceUpdater> updater) noexcept override;
        void add_mesh(const types::RenderHandle& mesh) noexcept override;
        foundation::DependencyNode get_node(void) const noexcept override;
        foundation::DependencyBounds get_dependencies(void) const noexcept override;

      private:
        void add_command(const types::RenderHandle handle);

        void add_shader(const types::RenderHandle& shader) noexcept override;
        foundation::UTF8 get_name(void) const noexcept override;
    };
} // namespace enishi::render_pass