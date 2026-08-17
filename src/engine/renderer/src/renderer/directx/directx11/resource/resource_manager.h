#pragma once
#include "../../../common/interface_gpu_resource_accessor.h"
#include "../../../common/interface_gpu_resource_maker.h"
#include "../../../errors/errors.h"
#include "../interface_d3d11_context.h"
#include "../shader/shader_refrection.h"
#include "gpu_resource.h"
#include <engine_types/renderer/mesh_handles.h>
#include <memory>
#include <platform/renderer/interface_image_view.h>
#include <platform/renderer/interface_pipeline_layout.h>
#include <unordered_map>

namespace enishi::renderer::directx {
    class ResourceManager : public GPUResourceMaker<DirectXError>,
                            public GPUResourceAccessor<DirectXError> {
      private:
        enum class ResourceType : std::uint32_t {
            Mesh,
            RenderTarget,
            DrawArgs,
            ShaderReflection,
        };

        struct ResourceIndex {
            ResourceType type;
            types::HandleId handle_id;

            bool operator==(const ResourceIndex& other) const {
                return this->type == other.type && this->handle_id == other.handle_id;
            }
        };

        struct KeyHash {
            std::size_t operator()(const ResourceIndex& k) const {
                constexpr auto SHIFT1 = sizeof(decltype(ResourceIndex::type)) * 8;
                // constexpr auto SHIFT2 = sizeof(decltype(ResourceIndex::handle_id)) * 8;
                return k.handle_id | (static_cast<std::size_t>(k.type) << SHIFT1);
            }
        };

      private:
        std::unique_ptr<types::HandleAllocator> handle_allocator;
        std::unique_ptr<GPUResource> native_resource;
        std::shared_ptr<ID3D11Context> context;
        std::unordered_map<types::HandleId, types::RenderHandleType> handles;
        std::unordered_map<types::HandleId, types::MeshHandles> meshes;
        std::unordered_map<ResourceIndex, std::size_t, KeyHash> handle_to_index;
        std::unordered_map<std::size_t, ResourceIndex> hash_to_shader_refection;
        std::vector<types::MeshHandles> meshes_;
        std::vector<types::DrawArgs> draw_args;
        std::vector<std::shared_ptr<platform::IRenderTargetView>> render_targets;
        std::vector<std::shared_ptr<ShaderReflection>> shader_refections;

      public:
        explicit ResourceManager(std::shared_ptr<ID3D11Context> context);

      public:
        [[nodiscard]] GPUResourceAccessor<DirectXError>* get_accessor(void);
        [[nodiscard]] const GPUResourceAccessor<DirectXError>* get_accessor(void) const;

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
        [[nodiscard]] foundation::Option<Buffer&> get_buffer(const types::HandleId handle);
        [[nodiscard]] foundation::Option<const Buffer&> get_buffer(
            const types::HandleId handle) const;
        [[nodiscard]] foundation::Option<const Texture&> get_texture(
            const types::HandleId handle) const;
        [[nodiscard]] foundation::Option<const Microsoft::WRL::ComPtr<ID3D11RasterizerState>&>
        get_rasterizer(const types::HandleId handle) const;
        [[nodiscard]] foundation::Option<const Microsoft::WRL::ComPtr<ID3D11InputLayout>&>
        get_input_layout(const types::HandleId handle) const;
        [[nodiscard]] foundation::Option<const types::MeshHandles&> get_mesh(
            const types::HandleId handle) const;
        [[nodiscard]] const ShaderPool& get_shader_pool(void) const;
        [[nodiscard]] const ViewPool& get_view_pool(void) const;
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