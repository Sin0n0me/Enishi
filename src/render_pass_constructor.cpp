#include "render_pass_constructor.h"
#include <foundation/log/logger.h>

namespace enishi {
    const std::filesystem::path SHADER_PATH = "./assets/shader";
    constexpr char PASS_MODEL[] = "ModelRenderPass";

    RenderPassConstructor::RenderPassConstructor(std::weak_ptr<platform::IRenderer> renderer,
        std::weak_ptr<assets_system::IAssetSystem> asset_system)
        : renderer(renderer)
        , asset_system(asset_system) {
    }

    RenderPassConstructor RenderPassConstructor::make(std::weak_ptr<platform::IRenderer> renderer,
        std::weak_ptr<assets_system::IAssetSystem> asset_system) {
        RenderPassConstructor constructor(renderer, asset_system);

        return constructor;
    }

    platform::RenderResult<types::RenderPass> RenderPassConstructor::make_model_render_pass(void) {
        auto renderer = this->renderer.lock();
        if (!bool(renderer)) {
            return foundation::Error(platform::RenderError::MakeError, "Renderer が存在しません");
        }

        types::PipelineDescription& description = this->name_to_description[PASS_MODEL];

        const auto vertex_shader_path = SHADER_PATH / "vs_model";
        const auto path = this->shader_paths.find();
        if (asset_paths.empty()) {
            return foundation::Error(
                platform::RenderError::MakeError, "シェーダーファイルが見つかりません");
        }

        description.shaders;

        return renderer->create_render_pass(description);
    }

    platform::RenderResult<void> RenderPassConstructor::find_shader(void) {
        auto asset_system = this->asset_system.lock();
        if (!bool(asset_system)) {
            return foundation::Error(
                platform::RenderError::MakeError, "Asset Systemが存在しません");
        }

        const auto asset_paths = asset_system->find_shaders("./assets/shader/");
        if (asset_paths.empty()) {
            return foundation::Error(
                platform::RenderError::MakeError, "シェーダーファイルが見つかりません");
        }

        this->shader_paths = std::unordered_set(asset_paths.begin(), asset_paths.end());

        return {};
    }
} // namespace enishi