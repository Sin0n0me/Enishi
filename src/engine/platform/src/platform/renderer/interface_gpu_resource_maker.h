#pragma once
#include <engine_types/assets/shader/shader_data.h>
#include <engine_types/assets/shader/shader_kind.h>
#include <engine_types/assets/texture/texture_data.h>
#include <engine_types/handle/handle_allocator.h>
#include <engine_types/handle/renderer/render_handle.h>
#include <engine_types/renderer/description/blend/blend_description.h>
#include <engine_types/renderer/description/depth/depth_stencil_description.h>
#include <engine_types/renderer/description/rasterizer/rasterizer_description.h>
#include <engine_types/renderer/description/sampler/sampler_description.h>
#include <engine_types/renderer/description/view/image_description.h>
#include <engine_types/renderer/description/view/image_view_description.h>
#include <engine_types/renderer/mesh/mesh_data.h>
#include <engine_types/renderer/render_data.h>
#include <engine_types/renderer/viewport.h>
#include <foundation/option/option.h>
#include <foundation/result/result.h>
#include <platform/errors/renderer_errors.h>

namespace enishi::platform {
    class GPUResourceMaker {
      public:
        virtual ~GPUResourceMaker(void) noexcept = default;

        [[nodiscard]] virtual foundation::Result<types::RenderHandle, RenderError>
        make_shader_reflection(const types::ShaderData& shader_data) = 0;
        [[nodiscard]] virtual foundation::Result<types::RenderHandle, RenderError>
        make_input_layout_from_shader_data(const types::ShaderData& shader_data) = 0;

        [[nodiscard]] virtual foundation::Result<types::RenderHandle, RenderError> make_mesh(
            types::MeshData&& mesh_data,
            const std::vector<types::RenderHandle>& shader_reflections) = 0;
        [[nodiscard]] virtual foundation::Result<types::RenderHandle, RenderError> make_shader(
            const types::ShaderKind kind, const types::ShaderData& shader_data) = 0;
        [[nodiscard]] virtual foundation::Result<types::RenderHandle, RenderError> make_texture(
            const types::TextureData& texture_data) = 0;
        [[nodiscard]] virtual foundation::Result<types::RenderHandle, RenderError>
        make_texture_from_render_data(const types::RenderData& data,
            const std::uint32_t width,
            const std::uint32_t height) = 0;
        [[nodiscard]] virtual foundation::Result<types::RenderHandle, RenderError>
        make_vertex_buffer(const types::RenderData& data) = 0;
        [[nodiscard]] virtual foundation::Result<types::RenderHandle, RenderError>
        make_index_buffer(const types::RenderData& data) = 0;
        [[nodiscard]] virtual foundation::Result<types::RenderHandle, RenderError>
        make_uniform_buffer(const types::RenderData& data) = 0;
        [[nodiscard]] virtual foundation::Result<types::RenderHandle, RenderError> make_image(
            const types::ImageDescription& description) = 0;
        [[nodiscard]] virtual foundation::Result<types::RenderHandle, RenderError> make_image(
            const types::TextureData& texture_data) = 0;

        [[nodiscard]] virtual foundation::Result<types::RenderHandle, RenderError> make_blend_state(
            const types::BlendStateDescription& description) = 0;
        [[nodiscard]] virtual foundation::Result<types::RenderHandle, RenderError>
        make_sampler_state(const types::SamplerStateDescription& description) = 0;
        [[nodiscard]] virtual foundation::Result<types::RenderHandle, RenderError>
        make_rasterizer_state(const types::RasterizerStateDescription& description) = 0;
        [[nodiscard]] virtual foundation::Result<types::RenderHandle, RenderError>
        make_depth_stencil_state(const types::DepthStencilStateDescription& description) = 0;

        [[nodiscard]] virtual foundation::Result<types::RenderHandle, RenderError> make_view(
            const types::RenderHandle& image_handle,
            const types::ImageViewDescription& description) = 0;
        [[nodiscard]] virtual foundation::Result<types::RenderHandle, RenderError> make_viewport(
            const types::ViewportRect& config) = 0;
    };
} // namespace enishi::platform