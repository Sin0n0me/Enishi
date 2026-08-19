#pragma once
#include "interface_native_buffer_accessor.h"
#include <cstdint>
#include <d3d11.h>
#include <engine_types/assets/shader/shader_kind.h>
#include <renderer/common/resource_pool.h>
#include <variant>
#include <wrl/client.h>

namespace enishi::renderer::directx {
    class BufferPool : public INativeBufferAccessor {
      private:
        ResourcePool<NativeBuffer> buffers;

      public:
        std::tuple<std::size_t, NativeBuffer&> make_native_buffer(void) noexcept override;
        void remove_native_buffer(const std::size_t index) noexcept override;
        foundation::Option<NativeBuffer&> get_native_buffer(const std::size_t index) noexcept override;
        foundation::Option<const NativeBuffer&> get_native_buffer(
            const std::size_t index) const noexcept override;
    };
} // namespace enishi::renderer::directx