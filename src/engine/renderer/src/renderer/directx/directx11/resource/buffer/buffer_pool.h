#pragma once
#include "interface_native_buffer_accessor.h"
#include <engine_types/handle/handle_mapper.h>
#include <platform/renderer/buffer/interface_buffer_accessor.h>
#include <platform/renderer/view/interface_view_accessor.h>
#include <renderer/common/resource_pool.h>

namespace enishi::renderer::directx {
    class BufferPool : public INativeBufferAccessor, public platform::IBufferAccessor {
      private:
        struct BufferHandle {
            std::size_t resource_index;
            std::size_t interface_index;
        };

      private:
        types::ResourceMapper<BufferHandle> handle_mapper;
        ResourcePool<NativeBuffer> native_buffers;
        ResourcePool<Buffer> buffers;

      public:
        std::tuple<types::HandleId, NativeBuffer&> make_native_buffer(void) noexcept override;
        void remove_native_buffer(const types::HandleId handle) noexcept override;
        foundation::Option<NativeBuffer&> get_native_buffer(
            const types::HandleId handle) noexcept override;
        foundation::Option<const NativeBuffer&> get_native_buffer(
            const types::HandleId handle) const noexcept override;

      public:
        std::tuple<types::HandleId, Buffer&> make_buffer(void) noexcept override;
        void add_interface(
            const types::HandleId handle, const Buffer buffer_interface) noexcept override;

        void remove_buffer(const types::HandleId handle) noexcept override;
        foundation::Option<Buffer&> get_buffer(const types::HandleId handle) noexcept override;
        foundation::Option<const Buffer&> get_bufer(
            const types::HandleId handle) const noexcept override;
    };
} // namespace enishi::renderer::directx