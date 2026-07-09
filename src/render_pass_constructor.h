#pragma once
#include <assets_system/interface_asset_system.h>
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
        std::unordered_set<std::filesystem::path> shader_paths;

      private:
        explicit RenderPassConstructor(std::weak_ptr<platform::IRenderer> renderer,
            std::weak_ptr<assets_system::IAssetSystem> asset_system);

      public:
        static RenderPassConstructor make(std::weak_ptr<platform::IRenderer> renderer,
            std::weak_ptr<assets_system::IAssetSystem> asset_system);

        platform::RenderResult<types::RenderPass> make_model_render_pass(void);

      private:
        platform::RenderResult<void> find_shader(void);
    };
} // namespace enishi