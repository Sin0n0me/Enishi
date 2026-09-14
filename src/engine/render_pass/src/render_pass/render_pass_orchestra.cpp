#include "render_pass_orchestra.h"
#include <filesystem>
#include <foundation/algorithm/resolve_dependencies.h>
#include <foundation/result/result.h>
#include <render_pass/constructor/interface_render_pass_construstor.h>
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

    void RenderPassOrchestra::make_render_passes(const platform::IWindow* window) {
        for (auto& constructor : this->constructors) {
            const auto pass_name = constructor->get_render_pass_name();
            if (this->name_to_pass.contains(pass_name)) {
                continue;
            }

            auto&& result =
                constructor->make(this->renderer.get(), window, this->shader_data_provider.get())
                    .add_message("");
            if (result.is_err()) {
                return;
            }

            this->name_to_pass.emplace(pass_name,
                RenderPassInfo{
                    .render_pass = result.unwrap(),
                });
        }
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

    foundation::VoidResult<ConstructError> RenderPassOrchestra::resoulve_mesh(
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