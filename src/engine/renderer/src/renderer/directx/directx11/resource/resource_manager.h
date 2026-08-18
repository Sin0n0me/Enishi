#pragma once
#include "../../../common/interface_gpu_resource_accessor.h"
#include "../../../common/interface_gpu_resource_maker.h"
#include "../../../errors/errors.h"
#include "../interface_d3d11_context.h"
#include "../shader/shader_refrection.h"
#include "interface_native_resouce_accessor.h"
#include "native_gpu_resource.h"
#include <engine_types/renderer/mesh_handles.h>
#include <memory>
#include <platform/renderer/interface_image_view.h>
#include <platform/renderer/interface_pipeline_layout.h>
#include <unordered_map>

namespace enishi::renderer::directx {
    class ResourceManager : public GPUResourceMaker<DirectXError>,
                            public GPUResourceAccessor<DirectXError> {
      private:
        std::unique_ptr<types::HandleAllocator> handle_allocator;
        std::unique_ptr<NativeGPUResource> native_resource;
        std::shared_ptr<ID3D11Context> context;
        std::unordered_map<types::RenderHandle, std::size_t> handle_to_index;

        std::unordered_map<types::HandleId, types::RenderHandleType> handles;
        std::unordered_map<types::HandleId, types::MeshHandles> meshes;
        std::unordered_map<std::size_t, types::RenderHandle> hash_to_shader_refection;
        std::vector<types::MeshHandles> meshes_;
        std::vector<types::DrawArgs> draw_args;
        std::vector<std::shared_ptr<platform::IRenderTargetView>> render_targets;
        std::vector<std::shared_ptr<ShaderReflection>> shader_refections;

      public:
        explicit ResourceManager(std::shared_ptr<ID3D11Context> context);

      public:
        [[nodiscard]] GPUResourceAccessor<DirectXError>* get_accessor(void);
        [[nodiscard]] const GPUResourceAccessor<DirectXError>* get_accessor(void) const;
        [[nodiscard]] INativeResourceAccessor* get_native_resource_accessor(void);
        [[nodiscard]] const INativeResourceAccessor* get_native_resource_accessor(void) const;

      public:
        foundation::Result<types::RenderHandle, DirectXError> make_shader_reflection(
            const types::ShaderData& shader_data) override;
        foundation::Result<types::RenderHandle, DirectXError> make_input_layout_from_shader_data(
            const types::ShaderData& shader_data) override;
        foundation::Result<types::RenderHandle, DirectXError> make_mesh(types::MeshData&& mesh_data,
            const std::vector<types::RenderHandle>& shader_reflections) override;
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
        foundation::Result<types::RenderHandle, DirectXError> make_uniform_buffer(
            const types::RenderData& data,
            const types::ShaderKind target_shader,
            const std::uint32_t target_slot) override;
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
        foundation::Option<std::shared_ptr<platform::IRenderTargetView>> get_render_target(
            const types::HandleId handle) const override;
        const std::vector<std::shared_ptr<platform::IRenderTargetView>>& get_render_targets(
            void) const override;
        foundation::Option<const types::DrawArgs&> get_draw_args(
            const types::HandleId handle) const override;
        foundation::Option<types::DrawArgs&> get_draw_args(const types::HandleId handle) override;
        foundation::Option<const IShaderReflection<DirectXError>*> get_shader_reflection(
            const types::HandleId handle) const override;
        foundation::Option<IShaderReflection<DirectXError>*> get_shader_reflection(
            const types::HandleId handle) override;

      public:
        // TODO: インターフェイス経由
        [[nodiscard]] foundation::Option<const Microsoft::WRL::ComPtr<ID3D11RasterizerState>&>
        get_rasterizer(const types::HandleId handle) const;
        [[nodiscard]] foundation::Option<const Microsoft::WRL::ComPtr<ID3D11InputLayout>&>
        get_input_layout(const types::HandleId handle) const;
        [[nodiscard]] foundation::Option<const types::MeshHandles&> get_mesh(
            const types::HandleId handle) const;
        [[nodiscard]] const std::vector<D3D11_VIEWPORT>& get_viewports(void) const;

      private:
        [[nodiscard]] foundation::Option<std::size_t> get_index(const ResourceIndex& index) const;

        [[nodiscard]] foundation::Result<types::RenderHandle, DirectXError> make_shader_from_dxbc(
            const types::ShaderKind kind, const types::ShaderData& shader_data);
        [[nodiscard]] foundation::VoidResult<DirectXError> make_vertex_shader(
            const types::ShaderData& shader_data, const types::HandleId handle);
        [[nodiscard]] foundation::VoidResult<DirectXError> make_pixel_shader(
            const types::ShaderData& shader_data, const types::HandleId handle);
    };
} // namespace enishi::renderer::directx