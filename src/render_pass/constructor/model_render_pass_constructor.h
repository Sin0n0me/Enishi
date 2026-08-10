#pragma once
#include <core/system/render/interface_render_pass_construstor.h>
#include <vector>

namespace enishi {
    class ModelRenderPassConstructor : public core::IRenderPassConstructor {
      public:
        foundation::Result<std::shared_ptr<platform::IRenderPass>, core::SystemError> make(
            platform::IRenderer* const renderer,
            assets_system::IAssetSystem* const asset_system) override;

      private:
        foundation::VoidResult<core::SystemError> make_render_target(
            types::PipelineDescription& description, platform::IRenderer* const renderer);
        foundation::VoidResult<core::SystemError> make_rasterizer(
            types::PipelineDescription& description, platform::IRenderer* const renderer);
        foundation::VoidResult<core::SystemError> make_input_layout(
            types::PipelineDescription& description,
            platform::IRenderer* const renderer,
            assets_system::IAssetSystem* const asset_system,
            const std::vector<std::filesystem::path>& asset_paths);
        foundation::VoidResult<core::SystemError> make_shaders(
            types::PipelineDescription& description,
            platform::IRenderer* const renderer,
            assets_system::IAssetSystem* const asset_system);
        foundation::Result<types::RenderHandle, core::SystemError> make_shader(
            const types::ShaderKind kind,
            const std::vector<std::filesystem::path>& asset_paths,
            platform::IRenderer* const renderer,
            assets_system::IAssetSystem* const asset_system);
        foundation::Result<types::RenderHandle, core::SystemError> make_mesh(
            platform::IRenderer* const renderer, assets_system::IAssetSystem* const asset_system);
    };
} // namespace enishi