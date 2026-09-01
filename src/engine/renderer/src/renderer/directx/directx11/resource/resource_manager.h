#pragma once
#include "../interface_d3d11_context.h"
#include "interface_native_resouce_accessor.h"
#include "native_gpu_resource.h"
#include <engine_types/assets/model/model_data.h>
#include <memory>
#include <platform/renderer/interface_gpu_resource_maker.h>
#include <platform/renderer/interface_pipeline_layout.h>
#include <platform/renderer/view/interface_image_view.h>
#include <renderer/common/resource_binder.h>
#include <renderer/common/updater/updater_pool.h>
#include <renderer/errors/errors.h>
#include <unordered_map>

namespace enishi::renderer::directx {
    class ResourceManager : public platform::GPUResourceMaker {
      public:
        struct ResourceHandles {
            types::HandleId resource;     // リソースそのもの
            types::HandleId binding;      // リソースをバインドするための
            types::HandleId configurable; // 外部から変更可能なインターフェイス
        };

        template <typename T> using Map = std::unordered_map<types::RenderHandle, T>;

      private:
        std::unique_ptr<types::HandleAllocator> handle_allocator;
        std::unique_ptr<NativeGPUResource> native_resource;
        std::unique_ptr<ResourceBinder> resource_binder;
        std::shared_ptr<ID3D11Context> context;
        Map<ResourceHandles> handle_mapper;

      public:
        explicit ResourceManager(std::shared_ptr<ID3D11Context> context);

      public:
        [[nodiscard]] foundation::Option<
            const decltype(ResourceManager::handle_mapper)::value_type::second_type&>
        get_native_resource_handle(const types::RenderHandle& handle) const noexcept;
        [[nodiscard]] platform::IGPUResourceAccessor* get_resource_accessor(void);
        [[nodiscard]] const platform::IGPUResourceAccessor* get_resource_accessor(void) const;
        [[nodiscard]] INativeResourceAccessor* get_native_resource_accessor(void);
        [[nodiscard]] const INativeResourceAccessor* get_native_resource_accessor(void) const;
        [[nodiscard]] platform::IResourceBinder* get_resource_binder(void);
        [[nodiscard]] const platform::IResourceBinder* get_resource_binder(void) const;

      public:
        foundation::Result<types::RenderHandle, platform::RenderError> make_shader_reflection(
            const types::ShaderData& shader_data) override;
        foundation::Result<types::RenderHandle, platform::RenderError>
        make_input_layout_from_shader_data(const types::ShaderData& shader_data) override;
        foundation::Result<types::RenderHandle, platform::RenderError> make_mesh(
            types::MeshData&& mesh_data,
            const std::vector<types::RenderHandle>& shader_reflections) override;
        foundation::Result<types::RenderHandle, platform::RenderError> make_shader(
            const types::ShaderKind kind, const types::ShaderData& shader_data) override;
        foundation::Result<types::RenderHandle, platform::RenderError> make_texture(
            const types::TextureData& texture_data) override;
        foundation::Result<types::RenderHandle, platform::RenderError>
        make_texture_from_render_data(const types::RenderData& data,
            const std::uint32_t width,
            const std::uint32_t height) override;
        foundation::Result<types::RenderHandle, platform::RenderError> make_vertex_buffer(
            const types::RenderData& data) override;
        foundation::Result<types::RenderHandle, platform::RenderError> make_index_buffer(
            const types::RenderData& data) override;
        foundation::Result<types::RenderHandle, platform::RenderError> make_uniform_buffer(
            const types::RenderData& data) override;
        foundation::Result<types::RenderHandle, platform::RenderError> make_image(
            const types::ImageDescription& description) override;
        foundation::Result<types::RenderHandle, platform::RenderError> make_image(
            const types::TextureData& texture_data) override;
        foundation::Result<types::RenderHandle, platform::RenderError> make_blend_state(
            const types::BlendStateDescription& description) override;
        foundation::Result<types::RenderHandle, platform::RenderError> make_sampler_state(
            const types::SamplerStateDescription& description) override;
        foundation::Result<types::RenderHandle, platform::RenderError> make_rasterizer_state(
            const types::RasterizerStateDescription& description) override;
        foundation::Result<types::RenderHandle, platform::RenderError> make_depth_stencil_state(
            const types::DepthStencilStateDescription& description) override;
        foundation::Result<types::RenderHandle, platform::RenderError> make_view(
            const types::RenderHandle& image_handle,
            const types::ImageViewDescription& description) override;
        foundation::Result<types::RenderHandle, platform::RenderError> make_viewport(
            const types::ViewportRect& config) override;

      private:
        [[nodiscard]] foundation::Result<types::RenderHandle, platform::RenderError>
        make_render_target_view(
            const ResourceHandles image_index, const types::ImageViewDescription& description);
        [[nodiscard]] foundation::Result<types::RenderHandle, platform::RenderError>
        make_depth_stencil_view(
            const ResourceHandles image_index, const types::ImageViewDescription& description);
        [[nodiscard]] foundation::Result<types::RenderHandle, platform::RenderError>
        make_shader_resource_view(
            const ResourceHandles image_index, const types::ImageViewDescription& description);
        [[nodiscard]] foundation::Result<types::RenderHandle, platform::RenderError>
        make_unodered_access_view(
            const ResourceHandles image_index, const types::ImageViewDescription& description);

        [[nodiscard]] foundation::Result<types::RenderHandle, platform::RenderError>
        make_shader_from_dxbc(const types::ShaderKind kind, const types::ShaderData& shader_data);
        [[nodiscard]] foundation::VoidResult<platform::RenderError> make_vertex_shader(
            const types::ShaderData& shader_data, const types::HandleId handle);
        [[nodiscard]] foundation::VoidResult<platform::RenderError> make_pixel_shader(
            const types::ShaderData& shader_data, const types::HandleId handle);
        [[nodiscard]] foundation::Result<std::vector<types::RenderHandle>, platform::RenderError>
        resolve_mesh_binding(types::MeshData&& mesh_data,
            std::vector<platform::IShaderAccessor::ShaderReflection>&& shader_reflections,
            types::HandleId&& mapped_index_buffer);
        [[nodiscard]] foundation::Result<std::vector<types::RenderHandle>, platform::RenderError>
        resolve_uniforms(types::MeshData::UniformMap&& uniforms,
            const std::vector<platform::IShaderAccessor::ShaderReflection>& shader_reflections);
        [[nodiscard]] foundation::Result<types::RenderHandle, platform::RenderError>
        resolve_uniform(const types::ShaderInputResource& input_resource,
            const types::ShaderKind& shader_kind,
            types::OwnedRenderData&& render_data);
        [[nodiscard]] foundation::Result<std::vector<types::RenderHandle>, platform::RenderError>
        resolve_texture(const types::MeshMaterial& mesh_material,
            const std::vector<platform::IShaderAccessor::ShaderReflection>& shader_reflections);
        [[nodiscard]] foundation::Result<foundation::Option<types::RenderHandle>,
            platform::RenderError>
        resolve_texture(const platform::IShaderAccessor::ShaderReflection& shader_reflection,
            const foundation::UTF8& target_name,
            const std::shared_ptr<types::TextureData>& texture);

        [[nodiscard]] std::vector<platform::IShaderAccessor::ShaderReflection>
        get_shader_reflections(const std::vector<types::RenderHandle>& shader_reflections) const;
    };
} // namespace enishi::renderer::directx