#include "opengl_resource_accessor.h"
#include <engine_types/handle/handle_allocator.h>

namespace enishi::renderer::opengl {
    std::tuple<types::HandleId, platform::IBufferAccessor::Buffer&>
    OpenGLResourceAccessor::make_buffer(void) noexcept {
        static types::HandleAllocator allocator;
        const auto handle = allocator.create();
        auto [it, _] = this->buffers.emplace(handle, nullptr);
        return {handle, it->second};
    }
    void OpenGLResourceAccessor::add_interface(
        const types::HandleId handle, const platform::IBufferAccessor::Buffer buffer) noexcept {
        this->buffers.insert_or_assign(handle, buffer);
    }
    foundation::Option<platform::IBufferAccessor::Buffer&> OpenGLResourceAccessor::get_buffer(
        const types::HandleId handle) noexcept {
        const auto it = this->buffers.find(handle);
        return it == this->buffers.end()
                   ? foundation::Option<platform::IBufferAccessor::Buffer&>()
                   : foundation::Option<platform::IBufferAccessor::Buffer&>(it->second);
    }
    foundation::Option<const platform::IBufferAccessor::Buffer&> OpenGLResourceAccessor::get_bufer(
        const types::HandleId handle) const noexcept {
        const auto it = this->buffers.find(handle);
        return it == this->buffers.end()
                   ? foundation::Option<const platform::IBufferAccessor::Buffer&>()
                   : foundation::Option<const platform::IBufferAccessor::Buffer&>(it->second);
    }
    foundation::Option<types::ImageViewType> OpenGLResourceAccessor::get_view_type(
        const types::HandleId& handle) const noexcept {
        if (this->render_targets.contains(handle))
            return types::ImageViewType::RenderTarget;
        if (this->depth_stencils.contains(handle))
            return types::ImageViewType::DepthStencil;
        if (this->shader_resources.contains(handle))
            return types::ImageViewType::ShaderResource;
        if (this->unordered_accesses.contains(handle))
            return types::ImageViewType::UnorderedAccess;
        return {};
    }
    types::HandleId OpenGLResourceAccessor::make_render_target_view(
        const types::HandleId& handle, RTV&& view) noexcept {
        this->render_target_list.push_back(view);
        this->render_targets.emplace(handle, std::move(view));
        return handle;
    }
    types::HandleId OpenGLResourceAccessor::make_shader_resource_view(
        const types::HandleId& handle, SRV&& view) noexcept {
        this->shader_resource_list.push_back(view);
        this->shader_resources.emplace(handle, std::move(view));
        return handle;
    }
    types::HandleId OpenGLResourceAccessor::make_depth_stencil_view(
        const types::HandleId& handle, DSV&& view) noexcept {
        this->depth_stencil_list.push_back(view);
        this->depth_stencils.emplace(handle, std::move(view));
        return handle;
    }
    types::HandleId OpenGLResourceAccessor::make_unordered_access_view(
        const types::HandleId& handle, UAV&& view) noexcept {
        this->unordered_access_list.push_back(view);
        this->unordered_accesses.emplace(handle, std::move(view));
        return handle;
    }
    foundation::Option<RTV&> OpenGLResourceAccessor::get_render_target_view(
        const types::HandleId handle) noexcept {
        const auto it = this->render_targets.find(handle);
        return it == this->render_targets.end() ? foundation::Option<RTV&>()
                                                : foundation::Option<RTV&>(it->second);
    }
    foundation::Option<const RTV&> OpenGLResourceAccessor::get_render_target_view(
        const types::HandleId handle) const noexcept {
        const auto it = this->render_targets.find(handle);
        return it == this->render_targets.end() ? foundation::Option<const RTV&>()
                                                : foundation::Option<const RTV&>(it->second);
    }
    foundation::Option<SRV&> OpenGLResourceAccessor::get_shader_resource_view(
        const types::HandleId handle) noexcept {
        const auto it = this->shader_resources.find(handle);
        return it == this->shader_resources.end() ? foundation::Option<SRV&>()
                                                  : foundation::Option<SRV&>(it->second);
    }
    foundation::Option<const SRV&> OpenGLResourceAccessor::get_shader_resource_view(
        const types::HandleId handle) const noexcept {
        const auto it = this->shader_resources.find(handle);
        return it == this->shader_resources.end() ? foundation::Option<const SRV&>()
                                                  : foundation::Option<const SRV&>(it->second);
    }
    foundation::Option<DSV&> OpenGLResourceAccessor::get_depth_stencil_view(
        const types::HandleId handle) noexcept {
        const auto it = this->depth_stencils.find(handle);
        return it == this->depth_stencils.end() ? foundation::Option<DSV&>()
                                                : foundation::Option<DSV&>(it->second);
    }
    foundation::Option<const DSV&> OpenGLResourceAccessor::get_depth_stencil_view(
        const types::HandleId handle) const noexcept {
        const auto it = this->depth_stencils.find(handle);
        return it == this->depth_stencils.end() ? foundation::Option<const DSV&>()
                                                : foundation::Option<const DSV&>(it->second);
    }
    foundation::Option<UAV&> OpenGLResourceAccessor::get_unordered_access_view(
        const types::HandleId handle) noexcept {
        const auto it = this->unordered_accesses.find(handle);
        return it == this->unordered_accesses.end() ? foundation::Option<UAV&>()
                                                    : foundation::Option<UAV&>(it->second);
    }
    foundation::Option<const UAV&> OpenGLResourceAccessor::get_unordered_access_view(
        const types::HandleId handle) const noexcept {
        const auto it = this->unordered_accesses.find(handle);
        return it == this->unordered_accesses.end() ? foundation::Option<const UAV&>()
                                                    : foundation::Option<const UAV&>(it->second);
    }
} // namespace enishi::renderer::opengl