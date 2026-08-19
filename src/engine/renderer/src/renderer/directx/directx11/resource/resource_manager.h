#pragma once
#include "../../../common/interface_gpu_resource_maker.h"
#include "../../../errors/errors.h"
#include "../interface_d3d11_context.h"
#include "interface_native_resouce_accessor.h"
#include "native_gpu_resource.h"
#include <engine_types/handle/renderer/handles/mesh_handles.h>
#include <memory>
#include <platform/renderer/interface_image_view.h>
#include <platform/renderer/interface_pipeline_layout.h>
#include <renderer/common/resource_binder.h>
#include <unordered_map>

namespace enishi::renderer::directx {
    class ResourceManager : public GPUResourceMaker<DirectXError> {
      public:
        struct ResourceIndex {
            static constexpr std::size_t INVALID_INDEX = 0xFFFF'FFFF;
            std::size_t resource = INVALID_INDEX;
            std::size_t binding = INVALID_INDEX;
            std::size_t configurable = INVALID_INDEX;
        };

      private:
        std::unique_ptr<types::HandleAllocator> handle_allocator;
        std::unique_ptr<NativeGPUResource> native_resource;
        std::unique_ptr<ResourceBinder> resource_binder;
        std::shared_ptr<ID3D11Context> context;
        std::unordered_map<types::RenderHandle, ResourceIndex> handle_to_index;

        std::unordered_map<types::HandleId, types::MeshHandles> meshes;
        std::unordered_map<std::size_t, types::RenderHandle> hash_to_shader_refection;
        std::vector<types::MeshHandles> meshes_;

      public:
        explicit ResourceManager(std::shared_ptr<ID3D11Context> context);

      public:
        [[nodiscard]] foundation::Option<
            const decltype(ResourceManager::handle_to_index)::value_type::second_type&>
        get_native_resource_index(const types::RenderHandle& handle) const noexcept;
        [[nodiscard]] IGPUResourceAccessor<DirectXError>* get_accessor(void);
        [[nodiscard]] const IGPUResourceAccessor<DirectXError>* get_accessor(void) const;
        [[nodiscard]] INativeResourceAccessor* get_native_resource_accessor(void);
        [[nodiscard]] const INativeResourceAccessor* get_native_resource_accessor(void) const;
        [[nodiscard]] IResourceBinder* get_resource_binder(void);
        [[nodiscard]] const IResourceBinder* get_resource_binder(void) const;

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
        foundation::Result<types::RenderHandle, DirectXError> make_view(
            const types::RenderHandle& image_handle,
            const types::ImageViewDescription& description) override;
        foundation::Result<types::RenderHandle, DirectXError> make_viewport(
            const types::ViewportRect& config) override;

      public:
        [[nodiscard]] foundation::Option<const types::MeshHandles&> get_mesh(
            const types::HandleId handle) const;

      private:
        [[nodiscard]] foundation::Result<types::RenderHandle, DirectXError> make_render_target(
            const ResourceIndex image_index, const types::ImageViewDescription& description);
        [[nodiscard]] foundation::Result<types::RenderHandle, DirectXError> make_depth_stencil(
            const ResourceIndex image_index, const types::ImageViewDescription& description);
        [[nodiscard]] foundation::Result<types::RenderHandle, DirectXError> make_shader_resource(
            const ResourceIndex image_index, const types::ImageViewDescription& description);
        [[nodiscard]] foundation::Result<types::RenderHandle, DirectXError> make_unodered_access(
            const ResourceIndex image_index, const types::ImageViewDescription& description);

        [[nodiscard]] foundation::Result<types::RenderHandle, DirectXError> make_shader_from_dxbc(
            const types::ShaderKind kind, const types::ShaderData& shader_data);
        [[nodiscard]] foundation::VoidResult<DirectXError> make_vertex_shader(
            const types::ShaderData& shader_data, const types::HandleId handle);
        [[nodiscard]] foundation::VoidResult<DirectXError> make_pixel_shader(
            const types::ShaderData& shader_data, const types::HandleId handle);
    };
} // namespace enishi::renderer::directx