#pragma once
#include "../../../errors/errors.h"
#include "../interface_d3d11_context.h"
#include "gpu_resource.h"
#include "mesh.h"
#include <engine_types/assets/shader/shader_data.h>
#include <engine_types/assets/shader/shader_kind.h>
#include <engine_types/assets/texture/texture_data.h>
#include <engine_types/handle/handle_allocator.h>
#include <engine_types/renderer/description/image_description.h>
#include <engine_types/renderer/description/image_view_description.h>
#include <engine_types/renderer/description/rasterizer_description.h>
#include <engine_types/renderer/description/sampler_description.h>
#include <engine_types/renderer/mesh_data.h>
#include <engine_types/renderer/render_data.h>
#include <engine_types/renderer/render_handle.h>
#include <engine_types/renderer/viewport.h>
#include <foundation/option/option.h>
#include <foundation/result/result.h>
#include <memory>
#include <platform/renderer/interface_image_view.h>
#include <platform/renderer/interface_pipeline_layout.h>
#include <unordered_map>

namespace enishi::renderer::directx {
    class ResourceManager {
      private:
        std::shared_ptr<ID3D11Context> context;
        types::HandleAllocator handle_allocator;
        std::unordered_map<types::HandleId, types::RenderHandleType> handles;
        std::unordered_map<types::HandleId, Mesh> meshes;
        GPUResource resource;

      public:
        explicit ResourceManager(std::shared_ptr<ID3D11Context> context);

      public:
        using Result = foundation::Result<types::RenderHandle, DirectXError>;

        [[nodiscard]] Result make_input_layout_from_shader(const types::ShaderData& shader_data);
        [[nodiscard]] Result make_mesh(const types::MeshData& mesh_data);
        [[nodiscard]] Result make_shader(
            const types::ShaderKind kind, const types::ShaderData& shader_data);
        [[nodiscard]] Result make_texture(const types::TextureData& texture_data);
        [[nodiscard]] Result make_vertex_buffer(const types::RenderData& data);
        [[nodiscard]] Result make_index_buffer(const types::RenderData& data);
        [[nodiscard]] Result make_constant_buffer(const types::RenderData& data);
        [[nodiscard]] Result make_texture(
            const types::RenderData& data, const std::uint32_t width, const std::uint32_t height);
        [[nodiscard]] Result make_image(const types::ImageDescription& description);
        [[nodiscard]] Result make_blend_state();
        [[nodiscard]] Result make_sampler();
        [[nodiscard]] Result make_rasterizer(const types::RasterizerDescription& description);
        [[nodiscard]] Result make_render_target_view(const types::RenderHandle& image_handle,
            const types::ImageViewDescription& description);
        [[nodiscard]] Result make_viewport(const types::ViewportRect& config);

        [[nodiscard]] foundation::Option<const Buffer&> get_buffer(
            const types::HandleId handle) const;
        [[nodiscard]] foundation::Option<const Texture&> get_texture(
            const types::HandleId handle) const;
        [[nodiscard]] foundation::Option<const Microsoft::WRL::ComPtr<ID3D11RasterizerState>&>
        get_rasterizer(const types::HandleId handle) const;
        [[nodiscard]] foundation::Option<const Microsoft::WRL::ComPtr<ID3D11InputLayout>&>
        get_input_layout(const types::HandleId handle) const;
        [[nodiscard]] foundation::Option<const Mesh&> get_mesh(const types::HandleId handle) const;
        [[nodiscard]] const ShaderPool& get_shader_pool(void) const;
        [[nodiscard]] const ViewPool& get_view_pool(void) const;
        [[nodiscard]] const std::vector<D3D11_VIEWPORT>& get_viewports(void) const;

      private:
        [[nodiscard]] Result make_shader_from_dxbc(
            const types::ShaderKind kind, const types::ShaderData& shader_data);
        [[nodiscard]] foundation::VoidResult<DirectXError> make_vertex_shader(

            const types::ShaderData& shader_data, const types::HandleId handle);
        [[nodiscard]] foundation::VoidResult<DirectXError> make_pixel_shader(

            const types::ShaderData& shader_data, const types::HandleId handle);
    };
} // namespace enishi::renderer::directx