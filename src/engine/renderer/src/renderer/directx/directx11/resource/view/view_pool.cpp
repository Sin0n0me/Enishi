#include "view_pool.h"
#include "depth_stencil_view.h"
#include "render_target_view.h"
#include "shader_resource_view.h"
#include "unodered_access_view.h"

namespace enishi::renderer::directx {
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

                auto opt_interface = this->depth_stencil.get(mapped_handle.interface_index);
                opt_interface.unwrap_mut().reset();
            } break;
            case types::ImageViewType::RenderTarget: {
            } break;
            case types::ImageViewType::ShaderResource: {
            } break;
            case types::ImageViewType::UnorderedAccess: {
            } break;
            default:
                return;
        }

        this->handle_mapper.remove(handle);
    }

    foundation::Option<types::ImageViewType> ViewPool::get_view_type(
        const types::HandleId& handle) const noexcept {
        const auto opt_view = this->handle_mapper.get(handle);
        if (opt_view.is_none()) {
            return {};
        }
        return opt_view.unwrap().type;
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

    types::HandleId ViewPool::make_render_target_view(
        const types::HandleId& handle, RenderTargetView&& rtv) noexcept {
        auto opt_view_handle = this->handle_mapper.get(handle);
        if (opt_view_handle.is_none()) {
            return {};
        }
        auto& view_handle = opt_view_handle.unwrap_mut();
        const auto [interface_index, _] = this->render_targets.emplace(std::move(rtv));
        view_handle.interface_index = interface_index;

        return handle;
    }
    types::HandleId ViewPool::make_shader_resource_view(
        const types::HandleId& handle, ShaderResourceView&& srv) noexcept {
        auto opt_view_handle = this->handle_mapper.get(handle);
        if (opt_view_handle.is_none()) {
            return {};
        }
        auto& view_handle = opt_view_handle.unwrap_mut();
        const auto [interface_index, _] = this->shader_resources.emplace(std::move(srv));
        view_handle.interface_index = interface_index;

        return handle;
    }
    types::HandleId ViewPool::make_depth_stencil_view(
        const types::HandleId& handle, DepthStencilView&& dsv) noexcept {
        auto opt_view_handle = this->handle_mapper.get(handle);
        if (opt_view_handle.is_none()) {
            return {};
        }
        auto& view_handle = opt_view_handle.unwrap_mut();
        const auto [interface_index, _] = this->depth_stencil.emplace(std::move(dsv));
        view_handle.interface_index = interface_index;

        return handle;
    }
    types::HandleId ViewPool::make_unodered_access_view(
        const types::HandleId& handle, UnorderedAccessView&& uav) noexcept {
        auto opt_view_handle = this->handle_mapper.get(handle);
        if (opt_view_handle.is_none()) {
            return {};
        }
        auto& view_handle = opt_view_handle.unwrap_mut();
        const auto [interface_index, _] = this->unodered_access.emplace(std::move(uav));
        view_handle.interface_index = interface_index;

        return handle;
    }
    foundation::Option<ViewPool::RenderTargetView&> ViewPool::get_render_target_view(
        const types::HandleId handle) noexcept {
        return this->handle_mapper.get(handle).and_then(
            [this](const decltype(handle_mapper)::ValueType& mapped_handle) {
                return this->render_targets.get(mapped_handle.interface_index);
            });
    }
    foundation::Option<const ViewPool::RenderTargetView&> ViewPool::get_render_target_view(
        const types::HandleId handle) const noexcept {
        return this->handle_mapper.get(handle).and_then(
            [this](const decltype(handle_mapper)::ValueType& mapped_handle) {
                return this->render_targets.get(mapped_handle.interface_index);
            });
    }
    foundation::Option<ViewPool::ShaderResourceView&> ViewPool::get_shader_resource_view(
        const types::HandleId handle) noexcept {
        return this->handle_mapper.get(handle).and_then(
            [this](const decltype(handle_mapper)::ValueType& mapped_handle) {
                return this->shader_resources.get(mapped_handle.interface_index);
            });
    }
    foundation::Option<const ViewPool::ShaderResourceView&> ViewPool::get_shader_resource_view(
        const types::HandleId handle) const noexcept {
        return this->handle_mapper.get(handle).and_then(
            [this](const decltype(handle_mapper)::ValueType& mapped_handle) {
                return this->shader_resources.get(mapped_handle.interface_index);
            });
    }
    foundation::Option<ViewPool::DepthStencilView&> ViewPool::get_depth_stencil_view(
        const types::HandleId handle) noexcept {
        return this->handle_mapper.get(handle).and_then(
            [this](const decltype(handle_mapper)::ValueType& mapped_handle) {
                return this->depth_stencil.get(mapped_handle.interface_index);
            });
    }
    foundation::Option<const ViewPool::DepthStencilView&> ViewPool::get_depth_stencil_view(
        const types::HandleId handle) const noexcept {
        return this->handle_mapper.get(handle).and_then(
            [this](const decltype(handle_mapper)::ValueType& mapped_handle) {
                return this->depth_stencil.get(mapped_handle.interface_index);
            });
    }
    foundation::Option<ViewPool::UnorderedAccessView&> ViewPool::get_unodered_access_view(
        const types::HandleId handle) noexcept {
        return this->handle_mapper.get(handle).and_then(
            [this](const decltype(handle_mapper)::ValueType& mapped_handle) {
                return this->unodered_access.get(mapped_handle.interface_index);
            });
    }
    foundation::Option<const ViewPool::UnorderedAccessView&> ViewPool::get_unodered_access_view(
        const types::HandleId handle) const noexcept {
        return this->handle_mapper.get(handle).and_then(
            [this](const decltype(handle_mapper)::ValueType& mapped_handle) {
                return this->unodered_access.get(mapped_handle.interface_index);
            });
    }
    std::span<const ViewPool::RenderTargetView> ViewPool::get_render_target_views(
        void) const noexcept {
        return this->render_targets.get_all();
    }
    std::span<const ViewPool::ShaderResourceView> ViewPool::get_shader_resource_views(
        void) const noexcept {
        return this->shader_resources.get_all();
    }
    std::span<const ViewPool::DepthStencilView> ViewPool::get_depth_stencil_views(
        void) const noexcept {
        return this->depth_stencil.get_all();
    }
    std::span<const ViewPool::UnorderedAccessView> ViewPool::get_unodered_access_views(
        void) const noexcept {
        return this->unodered_access.get_all();
    }
} // namespace enishi::renderer::directx