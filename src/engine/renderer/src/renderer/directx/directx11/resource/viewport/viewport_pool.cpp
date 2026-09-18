#include "viewport_pool.h"

namespace enishi::renderer::directx {
    std::tuple<types::HandleId, ViewportPool::NativeViewport&> ViewportPool::make_native_viewport(
        void) noexcept {
        return this->handle_mapper.make_from(
            this->native_viewports.make(), [](const std::size_t index) {
                return ViewportHandle{
                    .index = index,
                };
            });
    }

    void ViewportPool::remove_native_viewport(const types::HandleId handle) noexcept {
    }

    foundation::Option<ViewportPool::NativeViewport&> ViewportPool::get_native_viewport(
        const types::HandleId handle) noexcept {
        return this->handle_mapper.get(handle).and_then([this](const ViewportHandle viewpoty) {
            return this->native_viewports.get(viewpoty.index);
        });
    }

    foundation::Option<const ViewportPool::NativeViewport&> ViewportPool::get_native_viewport(
        const types::HandleId handle) const noexcept {
        return this->handle_mapper.get(handle).and_then([this](const ViewportHandle viewpoty) {
            return this->native_viewports.get(viewpoty.index);
        });
    }

    std::span<const ViewportPool::NativeViewport> ViewportPool::get_native_viewports(
        void) const noexcept {
        return this->native_viewports.get_all();
    }
} // namespace enishi::renderer::directx