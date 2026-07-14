#pragma once
#include <assets_system/interface_asset_system.h>
#include <assets_system/utility/path_objects.h>
#include <engine_types/assets/shader/shader_kind.h>
#include <foundation/result/result.h>
#include <foundation/str/str.h>
#include <platform/renderer/interface_renderer.h>
#include <unordered_map>

namespace enishi {
    class RenderPassConstructor {
      private:
        std::weak_ptr<platform::IRenderer> renderer;
        std::weak_ptr<assets_system::IAssetSystem> asset_system;
        std::unordered_map<foundation::UTF8, types::PipelineDescription> name_to_description;
        assets_system::PathObjects shader_paths;
        assets_system::PathObjects model_paths;

      private:
        explicit RenderPassConstructor(std::weak_ptr<platform::IRenderer> renderer,
            std::weak_ptr<assets_system::IAssetSystem> asset_system);

      public:
        static RenderPassConstructor make(std::weak_ptr<platform::IRenderer> renderer,
            std::weak_ptr<assets_system::IAssetSystem> asset_system);

        platform::RenderResult<types::RenderPass> make_model_render_pass(void);

      private:
        platform::RenderResult<void> find_shaders(void);
        platform::RenderResult<void> find_models(void);
        platform::RenderResult<types::RenderHandle> make_input_layout_from_shader(
            const std::regex& pattern,
            const std::shared_ptr<platform::IRenderer>& renderer,
            const std::shared_ptr<assets_system::IAssetSystem>& asset_system);
        platform::RenderResult<types::RenderHandle> make_shader(const types::ShaderKind kind,
            const std::regex& pattern,
            const std::shared_ptr<platform::IRenderer>& renderer,
            const std::shared_ptr<assets_system::IAssetSystem>& asset_system);
        platform::RenderResult<types::RenderHandle> make_mesh(const std::regex& pattern,
            const std::shared_ptr<platform::IRenderer>& renderer,
            const std::shared_ptr<assets_system::IAssetSystem>& asset_system);
    };
} // namespace enishi