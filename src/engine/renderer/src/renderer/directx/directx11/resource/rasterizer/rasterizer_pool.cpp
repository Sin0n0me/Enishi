#include "rasterizer_pool.h"

namespace enishi::renderer::directx {
    std::tuple<std::size_t, RasterizerPool::NativeRasterizer&> RasterizerPool::make_native_rasterizer(
        void) noexcept {
        return this->rasterizers.make();
    }

    void RasterizerPool::remove_native_rasterizer(const std::size_t index) noexcept {
    }

    foundation::Option<RasterizerPool::NativeRasterizer&> RasterizerPool::get_native_rasterizer(
        const std::size_t index) noexcept {
        return this->rasterizers.get(index);
    }

    foundation::Option<const RasterizerPool::NativeRasterizer&> RasterizerPool::get_native_rasterizer(
        const std::size_t index) const noexcept {
        return this->rasterizers.get(index);
    }
} // namespace enishi::renderer::directx