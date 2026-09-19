#pragma once

#include <platform/renderer/interface_render_resource_accessor.h>
#include <unordered_map>

namespace enishi::renderer::opengl {
    class OpenGLResourceAccessor final : public platform::IRenderResourceAccessor,
                                         public platform::IGPUResourceAccessor,
                                         public platform::IViewAccessor {
      private:
        using RTV = platform::IViewAccessor::RenderTargetView;
        using SRV = platform::IViewAccessor::ShaderResourceView;
        using DSV = platform::IViewAccessor::DepthStencilView;
        using UAV = platform::IViewAccessor::UnorderedAccessView;
        std::unordered_map<types::HandleId, RTV> render_targets;
        std::unordered_map<types::HandleId, SRV> shader_resources;
        std::unordered_map<types::HandleId, DSV> depth_stencils;
        std::unordered_map<types::HandleId, UAV> unordered_accesses;
        std::vector<RTV> render_target_list;
        std::vector<SRV> shader_resource_list;
        std::vector<DSV> depth_stencil_list;
        std::vector<UAV> unordered_access_list;
      public:
        platform::IGPUResourceAccessor* get_resource_accessor(void) noexcept override { return this; }
        const platform::IGPUResourceAccessor* get_resource_accessor(void) const noexcept override { return this; }
        platform::IViewAccessor* get_view_accessor(void) noexcept override { return this; }
        const platform::IViewAccessor* get_view_accessor(void) const noexcept override { return this; }
        platform::IShaderAccessor* get_shader_accessor(void) noexcept override { return nullptr; }
        const platform::IShaderAccessor* get_shader_accessor(void) const noexcept override { return nullptr; }
        platform::IMeshAccessor* get_mesh_accessor(void) noexcept override { return nullptr; }
        const platform::IMeshAccessor* get_mesh_accessor(void) const noexcept override { return nullptr; }
        platform::IStateAccessor* get_state_accessor(void) noexcept override { return nullptr; }
        const platform::IStateAccessor* get_state_accessor(void) const noexcept override { return nullptr; }
        platform::IBufferAccessor* get_buffer_accessor(void) noexcept override { return nullptr; }
        const platform::IBufferAccessor* get_buffer_accessor(void) const noexcept override { return nullptr; }
        foundation::Option<types::ImageViewType> get_view_type(const types::HandleId& handle) const noexcept override;
        types::HandleId make_render_target_view(const types::HandleId&, RTV&& view) noexcept override;
        types::HandleId make_shader_resource_view(const types::HandleId&, SRV&& view) noexcept override;
        types::HandleId make_depth_stencil_view(const types::HandleId&, DSV&& view) noexcept override;
        types::HandleId make_unordered_access_view(const types::HandleId&, UAV&& view) noexcept override;
        foundation::Option<RTV&> get_render_target_view(types::HandleId) noexcept override;
        foundation::Option<const RTV&> get_render_target_view(types::HandleId) const noexcept override;
        foundation::Option<SRV&> get_shader_resource_view(types::HandleId) noexcept override;
        foundation::Option<const SRV&> get_shader_resource_view(types::HandleId) const noexcept override;
        foundation::Option<DSV&> get_depth_stencil_view(types::HandleId) noexcept override;
        foundation::Option<const DSV&> get_depth_stencil_view(types::HandleId) const noexcept override;
        foundation::Option<UAV&> get_unordered_access_view(types::HandleId) noexcept override;
        foundation::Option<const UAV&> get_unordered_access_view(types::HandleId) const noexcept override;
        std::span<const RTV> get_render_target_views(void) const noexcept override { return this->render_target_list; }
        std::span<const SRV> get_shader_resource_views(void) const noexcept override { return this->shader_resource_list; }
        std::span<const DSV> get_depth_stencil_views(void) const noexcept override { return this->depth_stencil_list; }
        std::span<const UAV> get_unordered_access_views(void) const noexcept override { return this->unordered_access_list; }
    };
} // namespace enishi::renderer::opengl
