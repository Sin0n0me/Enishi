#pragma once
#include "interface_native_rasterizer_accessor.h"
#include <renderer/common/handle_mapper.h>
#include <renderer/common/resource_pool.h>

namespace enishi::renderer::directx {
    class RasterizerPool : public INativeRasterizerAccessor {
      private:
        struct RasterizerHandle {
            std::size_t resource_index;
            std::size_t interface_index;
        };

      private:
        HandleMapper<RasterizerHandle> handle_mapper;
        ResourcePool<NativeRasterizer> native_rasterizers;

      public:
        std::tuple<types::HandleId, NativeRasterizer&> make_native_rasterizer(
            void) noexcept override;
        void remove_native_rasterizer(const types::HandleId handle) noexcept override;
        foundation::Option<NativeRasterizer&> get_native_rasterizer(
            const types::HandleId handle) noexcept override;
        foundation::Option<const NativeRasterizer&> get_native_rasterizer(
            const types::HandleId handle) const noexcept override;
    };
} // namespace enishi::renderer::directx