#pragma once
#include "interface_render_pass_constructor.h"
#include <engine_types/renderer/description/blend/blend_description.h>
#include <engine_types/renderer/description/depth/depth_stencil_description.h>
#include <engine_types/renderer/description/rasterizer/rasterizer_description.h>
#include <engine_types/renderer/description/sampler/sampler_description.h>
#include <foundation/result/result.h>
#include <foundation/str/string_builder.h>
#include <memory>
#include <platform/asset/interface_shader_data_provider.h>
#include <platform/renderer/interface_render_pass.h>
#include <platform/renderer/interface_renderer.h>
#include <platform/window/interface_window.h>
#include <render_pass/errors/errors.h>
#include <vector>

namespace enishi::render_pass {
    class PipelineDescriptionConstructer {
      private:
        types::PipelineDescription description;
        platform::IRenderer* const renderer;
        const platform::IWindow* const window;
        foundation::StringBuilder errors;

      public:
        explicit PipelineDescriptionConstructer(
            platform::IRenderer* const renderer, const platform::IWindow* const window);

        [[nodiscard]] PipelineDescriptionConstructer* add_topology(
            const types::PrimitiveTopology topology) noexcept;

        [[nodiscard]] PipelineDescriptionConstructer* add_shaders(
            const platform::IShaderDataProvider* shader_data_provider,
            const std::span<const std::tuple<types::ShaderKind, std::filesystem::path>>
                shader_paths) noexcept;

        [[nodiscard]] PipelineDescriptionConstructer* add_render_target_view(
            const types::RenderHandle handle) noexcept;

        [[nodiscard]] PipelineDescriptionConstructer* add_render_target_view(
            const types::ImageFormat format) noexcept;

        [[nodiscard]] PipelineDescriptionConstructer* add_swap_chain_render_target_view(
            const types::ImageFormat format) noexcept;

        [[nodiscard]] PipelineDescriptionConstructer* add_depth_stencil_view(
            const types::RenderHandle handle) noexcept;

        [[nodiscard]] PipelineDescriptionConstructer* add_depth_stencil_view(
            const types::ImageFormat format) noexcept;

        [[nodiscard]] PipelineDescriptionConstructer* add_rasterizer_state(
            const types::RenderHandle handle) noexcept;

        [[nodiscard]] PipelineDescriptionConstructer* add_rasterizer_state(
            types::RasterizerStateDescription&& description) noexcept;

        [[nodiscard]] foundation::Result<types::PipelineDescription, ConstructError> build(
            void) noexcept;

        [[nodiscard]] foundation::Result<std::shared_ptr<platform::IRenderPass>, ConstructError>
        build_render_pass(IRenderPassConstructor* const constructor) noexcept;
    };
} // namespace enishi::render_pass