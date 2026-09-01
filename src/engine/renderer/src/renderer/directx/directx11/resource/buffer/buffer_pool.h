#pragma once
#include "interface_native_buffer_accessor.h"
#include <platform/renderer/view/interface_view_accessor.h>
#include <renderer/common/handle_mapper.h>
#include <renderer/common/resource_pool.h>

namespace enishi::renderer::directx {
    class BufferPool : public INativeBufferAccessor {
      private:
        struct BufferHandle {
            std::size_t resource_index;
            std::size_t interface_index;
        };

      private:
        HandleMapper<BufferHandle> handle_mapper;
        ResourcePool<NativeBuffer> native_buffers;
        ResourcePool<BufferInterface> buffer_interfaces;

      public:
        std::tuple<types::HandleId, NativeBuffer&> make_native_buffer(void) noexcept override;
        void remove_native_buffer(const types::HandleId handle) noexcept override;
        foundation::Option<NativeBuffer&> get_native_buffer(
            const types::HandleId handle) noexcept override;
        foundation::Option<const NativeBuffer&> get_native_buffer(
            const types::HandleId handle) const noexcept override;
        void add_interface(
            const types::HandleId handle, const BufferInterface buffer_interface) noexcept override;
        foundation::Option<BufferInterface&> get_buffer_interface(
            const types::HandleId handle) noexcept override;
        foundation::Option<const BufferInterface&> get_buffer_interface(
            const types::HandleId handle) const noexcept override;
    };
} // namespace enishi::renderer::directx