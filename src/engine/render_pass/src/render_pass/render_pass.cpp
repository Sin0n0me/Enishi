#include "render_pass.h"

namespace enishi::render_pass {
    std::span<const types::DrawCommand> RenderPass::get_commands(void) const noexcept {
        return this->commands;
    }

    types::RenderHandle RenderPass::get_render_target(void) const noexcept {
        return this->render_target;
    }

    std::span<const types::RenderHandle> RenderPass::get_shader_reflections(void) const noexcept {
        return this->shader_reflections;
    }

    void RenderPass::update(void) {
        for (auto& updater : this->resource_updater) {
            updater->on_update();
        }
    }

    foundation::VoidResult<platform::RenderError> RenderPass::make_from_description(
        const types::PipelineDescription& description,
        foundation::UTF8&& pass_name,
        foundation::DependencyNode&& node,
        foundation::DependencyBounds&& dependencies) noexcept {
        this->name = std::move(pass_name);
        this->shader_reflections = description.shader_reflections;

        // RTVの追加
        this->render_target = description.render_target_view;

        // DSVの追加
        this->add_command(description.render_target_view);
        this->add_command(description.depth_stencil_view);

        // トポロジの追加
        this->add_command(types::RenderHandle{
            types::HandleId{static_cast<types::HandleId::HandleType>(description.topology)},
            types::RenderHandleType::Topology,
        });

        // ラスタライザなどのステートの追加
        this->add_command(description.rasterizer_state);
        this->add_command(description.blend_state);
        this->add_command(description.depth_stencil_state);
        this->add_command(description.sampler_state);

        // 頂点レイアウトの追加
        this->add_command(description.vertex_layout);

        // シェーダーの追加
        for (const auto& shader : description.shaders) {
            this->add_command(shader);
        }

        this->dependencies = std::move(dependencies);
        this->node = std::move(node);

        return {};
    }

    void RenderPass::add_mesh(const foundation::UTF8& mesh_name, const types::RenderHandle handle) {
        this->mesh_name_to_index[mesh_name] = this->commands.size();
        this->add_command(handle);
    }

    void RenderPass::add_updater(std::shared_ptr<platform::IResourceUpdater> updater) noexcept {
        if (!bool(updater)) {
            return;
        }

        this->resource_updater.emplace_back(updater);
    }

    void RenderPass::add_command(const types::RenderHandle handle) {
        if (!handle.is_valid()) {
            return;
        }

        this->commands.emplace_back(types::DrawCommand{
            .handle = handle,
            .sub_command = types::SubCommand::Bind,
        });
    }

    void RenderPass::add_shader(const types::RenderHandle& shader) noexcept {
        this->add_command(shader);
    }

    foundation::UTF8 RenderPass::get_name(void) const noexcept {
        return this->name;
    }
    void RenderPass::add_mesh(const types::RenderHandle& mesh) noexcept {
        this->add_command(mesh);
    }
    foundation::DependencyNode RenderPass::get_node(void) const noexcept {
        return this->node;
    }
    foundation::DependencyBounds RenderPass::get_dependencies(void) const noexcept {
        return this->dependencies;
    }
} // namespace enishi::render_pass