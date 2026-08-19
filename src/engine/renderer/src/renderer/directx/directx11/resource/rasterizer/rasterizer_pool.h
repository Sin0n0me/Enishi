#pragma once
#include "interface_native_rasterizer_accessor.h"
#include <renderer/common/resource_pool.h>

namespace enishi::renderer::directx {
    class RasterizerPool : public INativeRasterizerAccessor {
      private:
        ResourcePool<NativeRasterizer> rasterizers;

      public:
        std::tuple<std::size_t, NativeRasterizer&> make_native_rasterizer(void) noexcept override;
        void remove_native_rasterizer(const std::size_t index) noexcept override;
        foundation::Option<NativeRasterizer&> get_native_rasterizer(const std::size_t index) noexcept override;
        foundation::Option<const NativeRasterizer&> get_native_rasterizer(
            const std::size_t index) const noexcept override;
    };
} // namespace enishi::renderer::directx