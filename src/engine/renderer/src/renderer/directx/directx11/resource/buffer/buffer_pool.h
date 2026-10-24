#pragma once
#include "interface_native_buffer_accessor.h"
#include <engine_types/handle/handle_mapper.h>
#include <foundation/pool/resource_pool.h>

namespace enishi::renderer::directx {
    class BufferPool : public INativeBufferAccessor {
      private:
        struct BufferHandle {
            std::size_t resource_index;
        };

      private:
        types::ResourceMapper<BufferHandle> handle_mapper;
        foundation::ResourcePool<NativeBuffer> native_buffers;

      public:
        std::tuple<types::HandleId, NativeBuffer&> make_native_buffer(void) noexcept override;
        void remove_native_buffer(const types::HandleId handle) noexcept override;
        foundation::Option<NativeBuffer&> get_native_buffer(
            const types::HandleId handle) noexcept override;
        foundation::Option<const NativeBuffer&> get_native_buffer(
            const types::HandleId handle) const noexcept override;
    };
} // namespace enishi::renderer::directx
