#pragma once
#include <engine_types/assets/shader/shader_data.h>
#include <engine_types/assets/shader/shader_kind.h>
#include <engine_types/assets/texture/texture_data.h>
#include <engine_types/handle/handle_allocator.h>
#include <engine_types/handle/renderer/render_handle.h>
#include <engine_types/renderer/description/image_description.h>
#include <engine_types/renderer/description/image_view_description.h>
#include <engine_types/renderer/description/rasterizer_description.h>
#include <engine_types/renderer/description/sampler_description.h>
#include <engine_types/renderer/mesh_data.h>
#include <engine_types/renderer/render_data.h>
#include <engine_types/renderer/viewport.h>
#include <foundation/option/option.h>
#include <foundation/result/result.h>

namespace enishi::renderer {
    template <typename E> class GPUResourceMaker {
      public:
        virtual ~GPUResourceMaker(void) noexcept = default;

        [[nodiscard]] virtual foundation::Result<types::RenderHandle, E> make_shader_reflection(
            const types::ShaderData& shader_data) = 0;
        [[nodiscard]] virtual foundation::Result<types::RenderHandle, E>
        make_input_layout_from_shader_data(const types::ShaderData& shader_data) = 0;

        [[nodiscard]] virtual foundation::Result<types::RenderHandle, E> make_mesh(
            types::MeshData&& mesh_data,
            const std::vector<types::RenderHandle>& shader_reflections) = 0;
        [[nodiscard]] virtual foundation::Result<types::RenderHandle, E> make_shader(
            const types::ShaderKind kind, const types::ShaderData& shader_data) = 0;
        [[nodiscard]] virtual foundation::Result<types::RenderHandle, E> make_texture(
            const types::TextureData& texture_data) = 0;
        [[nodiscard]] virtual foundation::Result<types::RenderHandle, E>
        make_texture_from_render_data(const types::RenderData& data,
            const std::uint32_t width,
            const std::uint32_t height) = 0;
        [[nodiscard]] virtual foundation::Result<types::RenderHandle, E> make_vertex_buffer(
            const types::RenderData& data) = 0;
        [[nodiscard]] virtual foundation::Result<types::RenderHandle, E> make_index_buffer(
            const types::RenderData& data) = 0;
        [[nodiscard]] virtual foundation::Result<types::RenderHandle, E> make_uniform_buffer(
            const types::RenderData& data,
            const types::ShaderKind target_shader,
            const std::uint32_t target_slot) = 0;
        [[nodiscard]] virtual foundation::Result<types::RenderHandle, E> make_image(
            const types::ImageDescription& description) = 0;

        [[nodiscard]] virtual foundation::Result<types::RenderHandle, E> make_blend_state() = 0;
        [[nodiscard]] virtual foundation::Result<types::RenderHandle, E> make_sampler() = 0;
        [[nodiscard]] virtual foundation::Result<types::RenderHandle, E> make_rasterizer(
            const types::RasterizerDescription& description) = 0;
        [[nodiscard]] virtual foundation::Result<types::RenderHandle, E> make_render_target_view(
            const types::RenderHandle& image_handle,
            const types::ImageViewDescription& description) = 0;
        [[nodiscard]] virtual foundation::Result<types::RenderHandle, E> make_viewport(
            const types::ViewportRect& config) = 0;
        [[nodiscard]] virtual foundation::Result<types::RenderHandle, E> make_draw_args(
            types::DrawArgs&& args) = 0;
    };
} // namespace enishi::renderer