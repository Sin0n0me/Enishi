#pragma once
#include "../../../common/interface_gpu_resource_accessor.h"
#include "../../../common/interface_gpu_resource_maker.h"
#include "../../../errors/errors.h"
#include "../interface_d3d11_context.h"
#include "gpu_resource.h"
#include "mesh.h"
#include "resource_editor.h"
#include <memory>
#include <platform/renderer/interface_image_view.h>
#include <platform/renderer/interface_pipeline_layout.h>
#include <unordered_map>

namespace enishi::renderer::directx {
    class ResourceManager : public GPUResourceMaker<DirectXError> {
      private:
        std::shared_ptr<ID3D11Context> context;
        types::HandleAllocator handle_allocator;
        std::unordered_map<types::HandleId, types::RenderHandleType> handles;
        std::unordered_map<types::HandleId, Mesh> meshes;
        std::unique_ptr<GPUResource> resource;
        std::unique_ptr<ResourceEditor> resource_editor;

      public:
        explicit ResourceManager(std::shared_ptr<ID3D11Context> context);

      public:
        GPUResourceAccessor* const get_accessor(void) const;

        // D3D11ResourceAccessor* const get_d3d11_accessor(void) const;

      public:
        foundation::Result<types::RenderHandle, DirectXError> make_input_layout_from_shader(
            const types::ShaderData& shader_data) override;
        foundation::Result<types::RenderHandle, DirectXError> make_mesh(
            types::MeshData&& mesh_data) override;
        foundation::Result<types::RenderHandle, DirectXError> make_shader(
            const types::ShaderKind kind, const types::ShaderData& shader_data) override;
        foundation::Result<types::RenderHandle, DirectXError> make_texture(
            const types::TextureData& texture_data) override;
        foundation::Result<types::RenderHandle, DirectXError> make_texture_from_render_data(
            const types::RenderData& data,
            const std::uint32_t width,
            const std::uint32_t height) override;
        foundation::Result<types::RenderHandle, DirectXError> make_vertex_buffer(
            const types::RenderData& data) override;
        foundation::Result<types::RenderHandle, DirectXError> make_index_buffer(
            const types::RenderData& data) override;
        foundation::Result<types::RenderHandle, DirectXError> make_constant_buffer(
            const types::RenderData& data) override;
        foundation::Result<types::RenderHandle, DirectXError> make_image(
            const types::ImageDescription& description) override;
        foundation::Result<types::RenderHandle, DirectXError> make_blend_state() override;
        foundation::Result<types::RenderHandle, DirectXError> make_sampler() override;
        foundation::Result<types::RenderHandle, DirectXError> make_rasterizer(
            const types::RasterizerDescription& description) override;
        foundation::Result<types::RenderHandle, DirectXError> make_render_target_view(
            const types::RenderHandle& image_handle,
            const types::ImageViewDescription& description) override;
        foundation::Result<types::RenderHandle, DirectXError> make_viewport(
            const types::ViewportRect& config) override;
        foundation::Result<types::RenderHandle, DirectXError> make_draw_args(
            types::DrawArgs&& args) override;

      public:
        // TODO: インターフェイス経由
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
        [[nodiscard]] foundation::Result<types::RenderHandle, DirectXError> make_shader_from_dxbc(
            const types::ShaderKind kind, const types::ShaderData& shader_data);
        [[nodiscard]] foundation::VoidResult<DirectXError> make_vertex_shader(
            const types::ShaderData& shader_data, const types::HandleId handle);
        [[nodiscard]] foundation::VoidResult<DirectXError> make_pixel_shader(
            const types::ShaderData& shader_data, const types::HandleId handle);
    };
} // namespace enishi::renderer::directx