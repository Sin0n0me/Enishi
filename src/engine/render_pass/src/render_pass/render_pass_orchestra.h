#pragma once
#include <filesystem>
#include <foundation/result/result.h>
#include <platform/asset/interface_asset_system.h>
#include <platform/asset/interface_shader_data_provider.h>
#include <render_pass/constructor/interface_render_pass_construstor.h>
#include <render_pass/errors/errors.h>
#include <span>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace enishi::render_pass {
    class RenderPassOrchestra {
      private:
        struct RenderPassInfo {
            std::shared_ptr<platform::IRenderPass> render_pass;
            foundation::Option<std::size_t> index;
        };

      private:
        std::shared_ptr<platform::IRenderer> renderer;
        std::vector<std::shared_ptr<platform::IRenderPass>> render_passes;
        std::unordered_map<foundation::UTF8, RenderPassInfo> name_to_pass;
        std::vector<std::shared_ptr<IRenderPassConstructor>> constructors;
        std::shared_ptr<platform::IShaderDataProvider> shader_data_provider;

      public:
        RenderPassOrchestra(std::shared_ptr<platform::IRenderer> renderer,
            std::shared_ptr<platform::IShaderDataProvider> shader_data_provider);
        RenderPassOrchestra(RenderPassOrchestra&&) = delete;

        void add_constructor(std::shared_ptr<IRenderPassConstructor> render_pass_constructor);

        std::span<const std::shared_ptr<IRenderPassConstructor>> get_constructors(void) const;

        foundation::VoidResult<ConstructError> make_render_passes(const platform::IWindow* window);

        foundation::Option<std::shared_ptr<platform::IRenderPass>> get_render_pass(
            const foundation::UTF8& pass_name);

        void set_render_passes(std::vector<foundation::UTF8>&& pass_names);

        void add_render_pass(const foundation::UTF8& pass_name);

        void remove_render_pass(const foundation::UTF8& pass_name);

        std::vector<std::shared_ptr<platform::IRenderPass>> get_passes(void) const;

      private:
        void silent_add_render_pass(const foundation::UTF8& pass_name);

        foundation::VoidResult<ConstructError> update_dependency(void);

        foundation::VoidResult<ConstructError> resoulve_mesh(
            platform::IRenderPass* const render_pass);
    };
} // namespace enishi::render_pass