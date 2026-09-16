#include "render_pass_orchestra.h"
#include <filesystem>
#include <foundation/algorithm/resolve_dependencies.h>
#include <foundation/result/result.h>
#include <render_pass/constructor/interface_render_pass_constructor.h>
#include <render_pass/errors/errors.h>
#include <span>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace enishi::render_pass {
    RenderPassOrchestra::RenderPassOrchestra(std::shared_ptr<platform::IRenderer> renderer,
        std::shared_ptr<platform::IShaderDataProvider> shader_data_provider)
        : renderer(renderer)
        , shader_data_provider(shader_data_provider) {
    }

    void RenderPassOrchestra::add_constructor(
        std::shared_ptr<IRenderPassConstructor> render_pass_constructor) {
        this->constructors.emplace_back(render_pass_constructor);
    }

    std::span<const std::shared_ptr<IRenderPassConstructor>> RenderPassOrchestra::get_constructors(
        void) const {
        return this->constructors;
    }

    foundation::Option<std::shared_ptr<platform::IRenderPass>> RenderPassOrchestra::get_render_pass(
        const foundation::UTF8& pass_name) {
        auto iter = this->name_to_pass.find(pass_name);
        if (iter == this->name_to_pass.end()) {
            return {};
        }
        return iter->second.render_pass;
    }

    foundation::VoidResult<ConstructError> RenderPassOrchestra::make_render_passes(
        const platform::IWindow* window) {
        std::vector<foundation::DependencyDescription> dependencies;
        dependencies.reserve(this->constructors.size());
        for (const auto& constructor : this->constructors) {
            dependencies.emplace_back(foundation::DependencyDescription{
                .node = constructor->get_node(),
                .bounds = constructor->get_dependencies(),
            });
        }

        auto&& dependency_result = foundation::resolve_dependencies(dependencies);
        if (dependency_result.is_err()) {
            return dependency_result.propagation(ConstructError::Construct);
        }

        std::unordered_map<foundation::DependencyNode, std::shared_ptr<platform::IRenderPass>>
            node_to_render_pass;
        for (const auto& index : dependency_result.unwrap()) {
            const auto& constructor = this->constructors[index];
            const auto pass_name = constructor->get_render_pass_name();
            if (this->name_to_pass.contains(pass_name)) {
                continue;
            }

            std::vector<platform::IRenderPass*> dependency_render_passes;
            for (const auto& precedent : constructor->get_dependencies().precedents) {
                const auto iterator = node_to_render_pass.find(precedent);
                if (iterator != node_to_render_pass.end()) {
                    dependency_render_passes.emplace_back(iterator->second.get());
                }
            }

            auto&& result = constructor->make(this->renderer.get(),
                window,
                this->shader_data_provider.get(),
                dependency_render_passes);
            if (result.is_err()) {
                return result.propagation(ConstructError::Construct);
            }

            auto render_pass = result.unwrap();
            node_to_render_pass.emplace(constructor->get_node(), render_pass);
            this->name_to_pass.emplace(pass_name, RenderPassInfo{.render_pass = std::move(render_pass)});
        }

        return {};
    }
    void RenderPassOrchestra::set_render_passes(std::vector<foundation::UTF8>&& pass_names) {
        for (const auto& name : pass_names) {
            this->silent_add_render_pass(name);
        }
        this->update_dependency();
    }

    void RenderPassOrchestra::add_render_pass(const foundation::UTF8& pass_name) {
        this->silent_add_render_pass(pass_name);
        this->update_dependency();
    }

    void RenderPassOrchestra::remove_render_pass(const foundation::UTF8& pass_name) {
    }

    std::vector<std::shared_ptr<platform::IRenderPass>> RenderPassOrchestra::get_passes(
        void) const {
        return this->render_passes;
    }

    foundation::VoidResult<ConstructError> RenderPassOrchestra::update_dependency(void) {
        auto passes{this->render_passes};

        // 依存解決
        std::vector<foundation::DependencyDescription> dependencies;
        for (const auto& pass : passes) {
            dependencies.emplace_back(foundation::DependencyDescription{
                .node = pass->get_node(),
                .bounds = pass->get_dependencies(),
            });
        }
        auto&& result = foundation::resolve_dependencies(dependencies)
                            .add_message("レンダーパスの依存解決に失敗しました");
        if (result.is_err()) {
            return result.propagation(ConstructError::Construct);
        }

        // 生成前に初期化
        this->render_passes.clear();
        for (auto& [name, info] : this->name_to_pass) {
            info.index = {};
        }

        // Topological order に従って RenderPass を生成
        auto& sorted_indices = result.unwrap();
        std::vector<std::shared_ptr<platform::IRenderPass>> render_passes;
        for (const auto& index : sorted_indices) {
            const auto& pass = passes[index];
            this->name_to_pass[pass->get_name()].index = this->render_passes.size();
            this->render_passes.emplace_back(pass);
        }

        return {};
    }

    void RenderPassOrchestra::silent_add_render_pass(const foundation::UTF8& pass_name) {
        const auto iter = this->name_to_pass.find(pass_name);
        if (iter == this->name_to_pass.end()) {
            return;
        }
        auto& pass_info = iter->second;

        if (pass_info.index.is_some()) {
            return;
        }

        pass_info.index = this->render_passes.size();
        this->render_passes.emplace_back(pass_info.render_pass);
    }

    foundation::VoidResult<ConstructError> RenderPassOrchestra::resolve_mesh(
        platform::IRenderPass* const render_pass) {
        auto mesh_handle = types::RenderHandle{};

        const auto& mapper = this->renderer->get_handle_mapper();
        auto view = this->renderer->get_resource_accessor()->get_resource_accessor();

        const auto opt_mapped_mesh_handle = mapper->get(mesh_handle);
        if (opt_mapped_mesh_handle.is_none()) {
            return foundation::Error(ConstructError::Construct);
        }
        const auto& mapped_mesh_handle = opt_mapped_mesh_handle.unwrap();
        const auto opt_mesh_handles =
            view->get_mesh_accessor()->get_mesh_handle(mapped_mesh_handle.resource);
        if (opt_mesh_handles.is_none()) {
            return foundation::Error(ConstructError::Construct);
        }

        render_pass->add_mesh(mesh_handle);

        // メッシュのバッファを外部から更新できるようにインターフェイスの取得
        const auto& mesh = opt_mesh_handles.unwrap();
        for (const auto& handle : mesh.mesh_handles) {
            const auto opt_buffer_handle = mapper->get(handle);
            if (opt_buffer_handle.is_none()) {
                return foundation::Error(ConstructError::Construct);
            }
            const auto& buffer_handle = opt_buffer_handle.unwrap();

            const auto& opt_buffer_interface =
                view->get_buffer_accessor()->get_bufer(buffer_handle.configurable);
            if (opt_buffer_interface.is_none()) {
                continue;
            }

            // レンダーパスに追加
            render_pass->add_updater(opt_buffer_interface.unwrap());
        }
    }
} // namespace enishi::render_pass