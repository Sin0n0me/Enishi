#pragma once
#include <engine_types/renderer/description/blend/blend_description.h>
#include <engine_types/renderer/description/depth/depth_stencil_description.h>
#include <engine_types/renderer/description/rasterizer/rasterizer_description.h>
#include <engine_types/renderer/description/sampler/sampler_description.h>
#include <foundation/result/result.h>
#include <memory>
#include <platform/asset/interface_shader_data_provider.h>
#include <platform/renderer/interface_render_pass.h>
#include <platform/renderer/interface_renderer.h>
#include <render_pass/errors/errors.h>
#include <vector>

namespace enishi::render_pass {
    using ShaderPaths = std::unordered_map<types::ShaderKind, std::vector<std::filesystem::path>>;

    struct ShaderResult {
        types::RenderHandle shader;
        types::RenderHandle shader_reflection;
        types::RenderHandle input_layout;
    };

    using ShaderKindToData =
        std::unordered_map<types::ShaderKind, std::vector<types::AssetShaderData>>;

    [[nodiscard]] ShaderKindToData make_shader_map_presorted(
        const std::vector<platform::ShaderDataEntry>& entries);

    [[nodiscard]] foundation::Result<types::RenderHandle, ConstructError> make_render_target(
        types::ImageDescription&& description,
        const types::ImageFormat view_format,
        platform::IRenderer* const renderer);

    [[nodiscard]] foundation::Result<types::RenderHandle, ConstructError> make_depth_stencil(
        types::ImageDescription&& description,
        const types::ImageFormat view_format,
        platform::IRenderer* const renderer);

    [[nodiscard]] foundation::Result<types::RenderHandle, ConstructError> make_shader_resource(
        types::ImageDescription&& description,
        const types::ImageFormat view_format,
        platform::IRenderer* const renderer);

    [[nodiscard]] foundation::Result<types::RenderHandle, ConstructError> make_shader_resource(
        types::ImageDescription&& description,
        const types::ImageFormat view_format,
        platform::IRenderer* const renderer);

    [[nodiscard]] foundation::Result<types::RenderHandle, ConstructError> make_rasterizer(
        types::RasterizerStateDescription&& description, platform::IRenderer* const renderer);

    [[nodiscard]] foundation::Result<types::RenderHandle, ConstructError> make_depth_stencil_state(
        types::DepthStencilStateDescription&& description, platform::IRenderer* const renderer);

    [[nodiscard]] foundation::Result<types::RenderHandle, ConstructError> make_blend_state(
        types::BlendStateDescription&& description, platform::IRenderer* const renderer);

    [[nodiscard]] foundation::Result<types::RenderHandle, ConstructError> make_sampler_state(
        types::SamplerStateDescription&& description, platform::IRenderer* const renderer);

    [[nodiscard]] foundation::Result<types::RenderHandle, ConstructError> make_input_layout(
        platform::IRenderer* const renderer, const std::vector<std::filesystem::path>& asset_paths);

    [[nodiscard]] foundation::Result<std::vector<ShaderResult>, ConstructError> make_shaders(
        platform::IRenderer* const renderer, ShaderKindToData&& shaders_map);

    [[nodiscard]] foundation::Result<ShaderResult, ConstructError> make_shader(
        const types::ShaderKind kind,
        const types::ShaderData& shader_data,
        platform::IRenderer* const renderer);
} // namespace enishi::render_pass