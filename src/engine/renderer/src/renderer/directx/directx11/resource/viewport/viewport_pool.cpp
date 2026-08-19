#include "viewport_pool.h"

namespace enishi::renderer::directx {
    std::tuple<std::size_t, ViewportPool::Viewport&> ViewportPool::make_native_viewport(void) noexcept {
        const auto index = this->viewports.size();
        auto viewport = Viewport{};
        this->viewports.emplace_back(viewport);
        return {index, viewport};
    }

    void ViewportPool::remove_native_viewport(const std::size_t index) noexcept {
    }

    foundation::Option<ViewportPool::Viewport&> ViewportPool::get_native_viewport(
        const std::size_t index) noexcept {
        if (this->viewports.size() < index + 1) {
            return {};
        }
        return this->viewports.at(index);
    }

    foundation::Option<const ViewportPool::Viewport&> ViewportPool::get_native_viewport(
        const std::size_t index) const noexcept {
        if (this->viewports.size() < index + 1) {
            return {};
        }
        return this->viewports.at(index);
    }

    std::span<const ViewportPool::Viewport> ViewportPool::get_native_viewports(void) const noexcept {
        return this->viewports;
    }
} // namespace enishi::renderer::directx