#include "buffer_pool.h"

namespace enishi::renderer::directx {
    std::tuple<types::HandleId, BufferPool::NativeBuffer&> BufferPool::make_native_buffer(
        void) noexcept {
        return this->handle_mapper.make_from(
            this->native_buffers.make(), [](const std::size_t index) {
                return decltype(handle_mapper)::ValueType{
                    .resource_index = index,
                };
            });
    }

    void BufferPool::remove_native_buffer(const types::HandleId handle) noexcept {
        const auto opt_mapped_handle = this->handle_mapper.get(handle);
        if (opt_mapped_handle.is_none()) {
            return;
        }
        const auto& mapped_handle = opt_mapped_handle.unwrap();
        auto opt = this->native_buffers.get(mapped_handle.resource_index);
        if (opt.is_none()) {
            return;
        }
        opt.unwrap_mut().Reset();

        this->handle_mapper.remove(handle);
    }

    foundation::Option<BufferPool::NativeBuffer&> BufferPool::get_native_buffer(
        const types::HandleId handle) noexcept {
        return this->handle_mapper.get(handle).and_then(
            [this](const decltype(handle_mapper)::ValueType mapped_handle) {
                return this->native_buffers.get(mapped_handle.resource_index);
            });
    }

    foundation::Option<const BufferPool::NativeBuffer&> BufferPool::get_native_buffer(
        const types::HandleId handle) const noexcept {
        return this->handle_mapper.get(handle).and_then(
            [this](const decltype(handle_mapper)::ValueType mapped_handle) {
                return this->native_buffers.get(mapped_handle.resource_index);
            });
    }
} // namespace enishi::renderer::directx