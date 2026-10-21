#include "view_pool.h"

namespace enishi::renderer::directx {
    ViewPool::ViewPool(ResourceAccessor& interfaces)
        : interfaces(interfaces) {
    }

    std::tuple<types::HandleId, ViewPool::NativeDepthStencilView&>
    ViewPool::make_native_depth_stencil_view(void) noexcept {
        return this->handle_mapper.make_from(
            this->native_depth_stencils.make(), [](const std::size_t index) {
                return decltype(handle_mapper)::ValueType{
                    .type = types::ImageViewType::DepthStencil,
                    .resource_index = index,
                };
            });
    }

    std::tuple<types::HandleId, ViewPool::NativeRenderTargetView&>
    ViewPool::make_native_render_target_view(void) noexcept {
        return this->handle_mapper.make_from(
            this->native_render_targets.make(), [](const std::size_t index) {
                return decltype(handle_mapper)::ValueType{
                    .type = types::ImageViewType::RenderTarget,
                    .resource_index = index,
                };
            });
    }

    std::tuple<types::HandleId, ViewPool::NativeShaderResourceView&>
    ViewPool::make_native_shader_resource_view(void) noexcept {
        return this->handle_mapper.make_from(
            this->native_shader_resources.make(), [](const std::size_t index) {
                return decltype(handle_mapper)::ValueType{
                    .type = types::ImageViewType::ShaderResource,
                    .resource_index = index,
                };
            });
    }

    std::tuple<types::HandleId, ViewPool::NativeUnorderedAccessView&>
    ViewPool::make_native_unordered_access_view(void) noexcept {
        return this->handle_mapper.make_from(
            this->native_unordered_accesses.make(), [](const std::size_t index) {
                return decltype(handle_mapper)::ValueType{
                    .type = types::ImageViewType::UnorderedAccess,
                    .resource_index = index,
                };
            });
    }

    void ViewPool::remove_native_view(const types::HandleId handle) noexcept {
        const auto opt_mapped_handle = this->handle_mapper.get(handle);
        if (opt_mapped_handle.is_none()) {
            return;
        }
        const auto& mapped_handle = opt_mapped_handle.unwrap();

        switch (mapped_handle.type) {
            case types::ImageViewType::DepthStencil: {
                auto opt_native_resource =
                    this->native_depth_stencils.get(mapped_handle.resource_index);
                if (opt_native_resource.is_none()) {
                    return;
                }
                opt_native_resource.unwrap_mut().Reset();

            } break;
            case types::ImageViewType::RenderTarget: {
                this->native_render_targets.get(mapped_handle.resource_index).unwrap_mut().Reset();
            } break;
            case types::ImageViewType::ShaderResource: {
                this->native_shader_resources.get(mapped_handle.resource_index)
                    .unwrap_mut()
                    .Reset();
            } break;
            case types::ImageViewType::UnorderedAccess: {
                this->native_unordered_accesses.get(mapped_handle.resource_index)
                    .unwrap_mut()
                    .Reset();
            } break;
            default:
                return;
        }

        this->interfaces.remove_view(handle);
        this->handle_mapper.remove(handle);
    }

    foundation::Option<const ViewPool::NativeDepthStencilView&>
    ViewPool::get_native_depth_stencil_view(const types::HandleId handle) const noexcept {
        return this->handle_mapper.get(handle).and_then(
            [this](const decltype(handle_mapper)::ValueType mapped_handle) {
                return this->native_depth_stencils.get(mapped_handle.resource_index);
            });
    }

    foundation::Option<ViewPool::NativeDepthStencilView&> ViewPool::get_native_depth_stencil_view(
        const types::HandleId handle) noexcept {
        return this->handle_mapper.get(handle).and_then(
            [this](const decltype(handle_mapper)::ValueType& mapped_handle) {
                return this->native_depth_stencils.get(mapped_handle.resource_index);
            });
    }

    foundation::Option<const ViewPool::NativeRenderTargetView&>
    ViewPool::get_native_render_target_view(const types::HandleId handle) const noexcept {
        return this->handle_mapper.get(handle).and_then(
            [this](const decltype(handle_mapper)::ValueType& mapped_handle) {
                return this->native_render_targets.get(mapped_handle.resource_index);
            });
    }

    foundation::Option<ViewPool::NativeRenderTargetView&> ViewPool::get_native_render_target_view(
        const types::HandleId handle) noexcept {
        return this->handle_mapper.get(handle).and_then(
            [this](const decltype(handle_mapper)::ValueType& mapped_handle) {
                return this->native_render_targets.get(mapped_handle.resource_index);
            });
    }

    foundation::Option<const ViewPool::NativeShaderResourceView&>
    ViewPool::get_native_shader_resource_view(const types::HandleId handle) const noexcept {
        return this->handle_mapper.get(handle).and_then(
            [this](const decltype(handle_mapper)::ValueType& mapped_handle) {
                return this->native_shader_resources.get(mapped_handle.resource_index);
            });
    }

    foundation::Option<ViewPool::NativeShaderResourceView&>
    ViewPool::get_native_shader_resource_view(const types::HandleId handle) noexcept {
        return this->handle_mapper.get(handle).and_then(
            [this](const decltype(handle_mapper)::ValueType& mapped_handle) {
                return this->native_shader_resources.get(mapped_handle.resource_index);
            });
    }

    foundation::Option<const ViewPool::NativeUnorderedAccessView&>
    ViewPool::get_native_unordered_access_view(const types::HandleId handle) const noexcept {
        return this->handle_mapper.get(handle).and_then(
            [this](const decltype(handle_mapper)::ValueType& mapped_handle) {
                return this->native_unordered_accesses.get(mapped_handle.resource_index);
            });
    }

    foundation::Option<ViewPool::NativeUnorderedAccessView&>
    ViewPool::get_native_unordered_access_view(const types::HandleId handle) noexcept {
        return this->handle_mapper.get(handle).and_then(
            [this](const decltype(handle_mapper)::ValueType& mapped_handle) {
                return this->native_unordered_accesses.get(mapped_handle.resource_index);
            });
    }

} // namespace enishi::renderer::directx
