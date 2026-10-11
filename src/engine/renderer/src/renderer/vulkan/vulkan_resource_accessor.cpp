#include "vulkan_resource_accessor.h"
#include <engine_types/handle/handle_allocator.h>

namespace enishi::renderer::vulkan {
    std::tuple<types::HandleId, VulkanResourceAccessor::ShaderReflection&>
    VulkanResourceAccessor::make_shader_reflection(ShaderReflection&& shader_reflection) noexcept {
        const auto handle = this->shader_reflections.emplace(std::move(shader_reflection));
        return {handle, this->shader_reflections.get(handle).unwrap_mut()};
    }
    foundation::Option<VulkanResourceAccessor::ShaderReflection&>
    VulkanResourceAccessor::get_shader_reflection(const types::HandleId handle) noexcept {
        return this->shader_reflections.get(handle);
    }
    foundation::Option<const VulkanResourceAccessor::ShaderReflection&>
    VulkanResourceAccessor::get_shader_reflection(const types::HandleId handle) const noexcept {
        return this->shader_reflections.get(handle);
    }
    void VulkanResourceAccessor::remove_shader_reflection(const types::HandleId handle) noexcept {
        this->shader_reflections.remove(handle);
    }

    std::tuple<types::HandleId, VulkanResourceAccessor::MeshHandles&>
    VulkanResourceAccessor::make_mesh_handles(void) noexcept {
        const auto handle = this->mesh_handles.make();
        return {handle, this->mesh_handles.get(handle).unwrap_mut()};
    }
    void VulkanResourceAccessor::remove_mesh_handles(const types::HandleId handle) noexcept {
        this->mesh_handles.remove(handle);
    }
    foundation::Option<VulkanResourceAccessor::MeshHandles&>
    VulkanResourceAccessor::get_mesh_handle(const types::HandleId handle) noexcept {
        return this->mesh_handles.get(handle);
    }
    foundation::Option<const VulkanResourceAccessor::MeshHandles&>
    VulkanResourceAccessor::get_mesh_handle(const types::HandleId handle) const noexcept {
        return this->mesh_handles.get(handle);
    }

    foundation::Option<types::StateKind> VulkanResourceAccessor::get_state_kind(
        const types::HandleId& handle) const noexcept {
        const auto it = this->state_kinds.find(handle);
        return it == this->state_kinds.end() ? foundation::Option<types::StateKind>()
                                             : foundation::Option<types::StateKind>(it->second);
    }
    void VulkanResourceAccessor::add_state(
        const types::HandleId handle, const types::StateKind kind) noexcept {
        this->state_kinds.insert_or_assign(handle, kind);
    }

    std::tuple<types::HandleId, platform::IBufferAccessor::Buffer&>
    VulkanResourceAccessor::make_buffer(void) noexcept {
        static types::HandleAllocator allocator;
        const auto handle = allocator.create();
        auto [it, _] = this->buffers.emplace(handle, nullptr);
        return {handle, it->second};
    }
    void VulkanResourceAccessor::add_interface(
        const types::HandleId handle, const platform::IBufferAccessor::Buffer buffer) noexcept {
        this->buffers.insert_or_assign(handle, buffer);
    }
    foundation::Option<platform::IBufferAccessor::Buffer&> VulkanResourceAccessor::get_buffer(
        const types::HandleId handle) noexcept {
        const auto it = this->buffers.find(handle);
        return it == this->buffers.end()
                   ? foundation::Option<platform::IBufferAccessor::Buffer&>()
                   : foundation::Option<platform::IBufferAccessor::Buffer&>(it->second);
    }
    foundation::Option<const platform::IBufferAccessor::Buffer&> VulkanResourceAccessor::get_bufer(
        const types::HandleId handle) const noexcept {
        const auto it = this->buffers.find(handle);
        return it == this->buffers.end()
                   ? foundation::Option<const platform::IBufferAccessor::Buffer&>()
                   : foundation::Option<const platform::IBufferAccessor::Buffer&>(it->second);
    }
    foundation::Option<types::ImageViewType> VulkanResourceAccessor::get_view_type(
        const types::HandleId& handle) const noexcept {
        if (this->render_targets.contains(handle)) {
            return types::ImageViewType::RenderTarget;
        }
        if (this->depth_stencils.contains(handle)) {
            return types::ImageViewType::DepthStencil;
        }
        if (this->shader_resources.contains(handle)) {
            return types::ImageViewType::ShaderResource;
        }
        if (this->unordered_accesses.contains(handle)) {
            return types::ImageViewType::UnorderedAccess;
        }
        return {};
    }
    types::HandleId VulkanResourceAccessor::make_render_target_view(
        const types::HandleId& handle, RTV&& view) noexcept {
        this->render_target_list.push_back(view);
        this->render_targets.emplace(handle, std::move(view));
        return handle;
    }
    types::HandleId VulkanResourceAccessor::make_shader_resource_view(
        const types::HandleId& handle, SRV&& view) noexcept {
        this->shader_resource_list.push_back(view);
        this->shader_resources.emplace(handle, std::move(view));
        return handle;
    }
    types::HandleId VulkanResourceAccessor::make_depth_stencil_view(
        const types::HandleId& handle, DSV&& view) noexcept {
        this->depth_stencil_list.push_back(view);
        this->depth_stencils.emplace(handle, std::move(view));
        return handle;
    }
    types::HandleId VulkanResourceAccessor::make_unordered_access_view(
        const types::HandleId& handle, UAV&& view) noexcept {
        this->unordered_access_list.push_back(view);
        this->unordered_accesses.emplace(handle, std::move(view));
        return handle;
    }
    foundation::Option<VulkanResourceAccessor::RTV&> VulkanResourceAccessor::get_render_target_view(
        const types::HandleId handle) noexcept {
        const auto it = this->render_targets.find(handle);
        return it == this->render_targets.end() ? foundation::Option<RTV&>()
                                                : foundation::Option<RTV&>(it->second);
    }
    foundation::Option<const VulkanResourceAccessor::RTV&>
    VulkanResourceAccessor::get_render_target_view(const types::HandleId handle) const noexcept {
        const auto it = this->render_targets.find(handle);
        return it == this->render_targets.end() ? foundation::Option<const RTV&>()
                                                : foundation::Option<const RTV&>(it->second);
    }
    foundation::Option<VulkanResourceAccessor::SRV&>
    VulkanResourceAccessor::get_shader_resource_view(const types::HandleId handle) noexcept {
        const auto it = this->shader_resources.find(handle);
        return it == this->shader_resources.end() ? foundation::Option<SRV&>()
                                                  : foundation::Option<SRV&>(it->second);
    }
    foundation::Option<const VulkanResourceAccessor::SRV&>
    VulkanResourceAccessor::get_shader_resource_view(const types::HandleId handle) const noexcept {
        const auto it = this->shader_resources.find(handle);
        return it == this->shader_resources.end() ? foundation::Option<const SRV&>()
                                                  : foundation::Option<const SRV&>(it->second);
    }
    foundation::Option<VulkanResourceAccessor::DSV&> VulkanResourceAccessor::get_depth_stencil_view(
        const types::HandleId handle) noexcept {
        const auto it = this->depth_stencils.find(handle);
        return it == this->depth_stencils.end() ? foundation::Option<DSV&>()
                                                : foundation::Option<DSV&>(it->second);
    }
    foundation::Option<const VulkanResourceAccessor::DSV&>
    VulkanResourceAccessor::get_depth_stencil_view(const types::HandleId handle) const noexcept {
        const auto it = this->depth_stencils.find(handle);
        return it == this->depth_stencils.end() ? foundation::Option<const DSV&>()
                                                : foundation::Option<const DSV&>(it->second);
    }
    foundation::Option<VulkanResourceAccessor::UAV&>
    VulkanResourceAccessor::get_unordered_access_view(const types::HandleId handle) noexcept {
        const auto it = this->unordered_accesses.find(handle);
        return it == this->unordered_accesses.end() ? foundation::Option<UAV&>()
                                                    : foundation::Option<UAV&>(it->second);
    }
    foundation::Option<const VulkanResourceAccessor::UAV&>
    VulkanResourceAccessor::get_unordered_access_view(const types::HandleId handle) const noexcept {
        const auto it = this->unordered_accesses.find(handle);
        return it == this->unordered_accesses.end() ? foundation::Option<const UAV&>()
                                                    : foundation::Option<const UAV&>(it->second);
    }

    platform::IGPUResourceAccessor* VulkanResourceAccessor::get_resource_accessor(void) noexcept {
        return this;
    }
    const platform::IGPUResourceAccessor* VulkanResourceAccessor::get_resource_accessor(
        void) const noexcept {
        return this;
    }
    platform::IViewAccessor* VulkanResourceAccessor::get_view_accessor(void) noexcept {
        return this;
    }
    const platform::IViewAccessor* VulkanResourceAccessor::get_view_accessor(void) const noexcept {
        return this;
    }
    platform::IShaderAccessor* VulkanResourceAccessor::get_shader_accessor(void) noexcept {
        return this;
    }
    const platform::IShaderAccessor* VulkanResourceAccessor::get_shader_accessor(
        void) const noexcept {
        return this;
    }
    platform::IMeshAccessor* VulkanResourceAccessor::get_mesh_accessor(void) noexcept {
        return this;
    }
    const platform::IMeshAccessor* VulkanResourceAccessor::get_mesh_accessor(void) const noexcept {
        return this;
    }
    platform::IStateAccessor* VulkanResourceAccessor::get_state_accessor(void) noexcept {
        return this;
    }
    const platform::IStateAccessor* VulkanResourceAccessor::get_state_accessor(
        void) const noexcept {
        return this;
    }
    platform::IBufferAccessor* VulkanResourceAccessor::get_buffer_accessor(void) noexcept {
        return this;
    }
    const platform::IBufferAccessor* VulkanResourceAccessor::get_buffer_accessor(
        void) const noexcept {
        return this;
    }
    std::span<const VulkanResourceAccessor::RTV> VulkanResourceAccessor::get_render_target_views(
        void) const noexcept {
        return this->render_target_list;
    }
    std::span<const VulkanResourceAccessor::SRV> VulkanResourceAccessor::get_shader_resource_views(
        void) const noexcept {
        return this->shader_resource_list;
    }
    std::span<const VulkanResourceAccessor::DSV> VulkanResourceAccessor::get_depth_stencil_views(
        void) const noexcept {
        return this->depth_stencil_list;
    }
    std::span<const VulkanResourceAccessor::UAV> VulkanResourceAccessor::get_unordered_access_views(
        void) const noexcept {
        return this->unordered_access_list;
    }
    void VulkanResourceAccessor::remove_buffer(types::HandleId handle) noexcept {
        this->buffers.erase(handle);
    }
} // namespace enishi::renderer::vulkan
