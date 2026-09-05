#pragma once
#include <assets_system/interface_asset_system.h>
#include <core/errors/errors.h>
#include <engine_types/renderer/description/blend/blend_description.h>
#include <engine_types/renderer/description/depth/depth_stencil_description.h>
#include <engine_types/renderer/description/rasterizer/rasterizer_description.h>
#include <engine_types/renderer/description/sampler/sampler_description.h>
#include <foundation/result/result.h>
#include <memory>
#include <platform/renderer/interface_render_pass.h>
#include <platform/renderer/interface_renderer.h>
#include <vector>

namespace enishi {
    using ShaderPaths = std::unordered_map<types::ShaderKind, std::vector<std::filesystem::path>>;

    struct ShaderResult {
        types::RenderHandle shader;
        types::RenderHandle shader_reflection;
        types::RenderHandle input_layout;
    };

    [[nodiscard]] foundation::Result<types::RenderHandle, core::SystemError> make_render_target(
        types::ImageDescription&& description,
        types::ImageFormat&& view_format,
        platform::IRenderer* const renderer);

    [[nodiscard]] foundation::Result<types::RenderHandle, core::SystemError> make_depth_stencil(
        types::ImageDescription&& description,
        types::ImageFormat&& view_format,
        platform::IRenderer* const renderer);

    [[nodiscard]] foundation::Result<types::RenderHandle, core::SystemError> make_shader_resource(
        types::ImageDescription&& description,
        types::ImageFormat&& view_format,
        platform::IRenderer* const renderer);

    [[nodiscard]] foundation::Result<types::RenderHandle, core::SystemError> make_shader_resource(
        types::ImageDescription&& description,
        types::ImageFormat&& view_format,
        platform::IRenderer* const renderer);

    [[nodiscard]] foundation::Result<types::RenderHandle, core::SystemError> make_rasterizer(
        types::RasterizerStateDescription&& description, platform::IRenderer* const renderer);

    [[nodiscard]] foundation::Result<types::RenderHandle, core::SystemError>
    make_depth_stencil_state(
        types::DepthStencilStateDescription&& description, platform::IRenderer* const renderer);

    [[nodiscard]] foundation::Result<types::RenderHandle, core::SystemError> make_blend_state(
        types::BlendStateDescription&& description, platform::IRenderer* const renderer);

    [[nodiscard]] foundation::Result<types::RenderHandle, core::SystemError> make_sampler_state(
        types::SamplerStateDescription&& description, platform::IRenderer* const renderer);

    [[nodiscard]] foundation::Result<types::RenderHandle, core::SystemError> make_input_layout(
        platform::IRenderer* const renderer,
        assets_system::IAssetSystem* const asset_system,
        const std::vector<std::filesystem::path>& asset_paths);

    [[nodiscard]] foundation::Result<ShaderResult, core::SystemError> make_shader_from_file_paths(
        const types::ShaderKind kind,
        const std::vector<std::filesystem::path>& paths,
        platform::IRenderer* const renderer,
        assets_system::IAssetSystem* const asset_system);

    [[nodiscard]] foundation::Result<std::vector<ShaderResult>, core::SystemError> make_shaders(
        platform::IRenderer* const renderer,
        assets_system::IAssetSystem* const asset_system,
        ShaderPaths&& paths);

    [[nodiscard]] foundation::Result<ShaderResult, core::SystemError> make_shader(
        const types::ShaderKind kind,
        const std::filesystem::path& shader_path,
        platform::IRenderer* const renderer,
        assets_system::IAssetSystem* const asset_system);
} // namespace enishi