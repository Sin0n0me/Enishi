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

    std::tuple<types::HandleId, BufferPool::Buffer&> BufferPool::make_buffer(void) noexcept {
        return this->handle_mapper.make_from(this->buffers.make(), [](const std::size_t index) {
            return decltype(handle_mapper)::ValueType{
                .interface_index = index,
            };
        });
    }

    void BufferPool::add_interface(
        const types::HandleId handle, const Buffer buffer_interface) noexcept {
        auto opt_mapped_handle = this->handle_mapper.get(handle);
        if (opt_mapped_handle.is_none()) {
            return;
        }
        auto& mapped_handle = opt_mapped_handle.unwrap_mut();

        auto [index, _] = this->buffers.make(buffer_interface);
        mapped_handle.interface_index = index;
    }

    void BufferPool::remove_buffer(const types::HandleId handle) noexcept {
    }

    foundation::Option<BufferPool::Buffer&> BufferPool::get_buffer(
        const types::HandleId handle) noexcept {
        return this->handle_mapper.get(handle).and_then(
            [this](const decltype(handle_mapper)::ValueType mapped_handle) {
                return this->buffers.get(mapped_handle.interface_index);
            });
    }

    foundation::Option<const BufferPool::Buffer&> BufferPool::get_bufer(
        const types::HandleId handle) const noexcept {
        return this->handle_mapper.get(handle).and_then(
            [this](const decltype(handle_mapper)::ValueType mapped_handle) {
                return this->buffers.get(mapped_handle.interface_index);
            });
    }
} // namespace enishi::renderer::directx