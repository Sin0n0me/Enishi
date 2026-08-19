#include "buffer_pool.h"

namespace enishi::renderer::directx {
    std::tuple<std::size_t, BufferPool::NativeBuffer&> BufferPool::make_native_buffer(void) noexcept {
        return this->buffers.make();
    }

    void BufferPool::remove_native_buffer(const std::size_t index) noexcept {
        auto opt = this->buffers.get(index);
        if (opt.is_none()) {
            return;
        }
        opt.unwrap_mut().Reset();
    }

    foundation::Option<BufferPool::NativeBuffer&> BufferPool::get_native_buffer(
        const std::size_t index) noexcept {
        return this->buffers.get(index);
    }

    foundation::Option<const BufferPool::NativeBuffer&> BufferPool::get_native_buffer(
        const std::size_t index) const noexcept {
        return this->buffers.get(index);
    }
} // namespace enishi::renderer::directx