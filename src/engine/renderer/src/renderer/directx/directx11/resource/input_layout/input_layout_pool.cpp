#include "input_layout_pool.h"

namespace enishi::renderer::directx {
    std::tuple<types::HandleId, InputLayoutPool::NativeInputLayout&>
    InputLayoutPool::make_native_input_layout(void) noexcept {
        return this->handle_mapper.make_from(
            this->native_input_layouts.make(), [](const std::size_t index) {
                return decltype(handle_mapper)::ValueType{
                    .resource_index = index,
                };
            });
    }

    void InputLayoutPool::remove_native_input_layout(const types::HandleId handle) noexcept {
        const auto opt_mapped_handle = this->handle_mapper.get(handle);
        if (opt_mapped_handle.is_none()) {
            return;
        }
        const auto& mapped_handle = opt_mapped_handle.unwrap();

        auto opt_native_resource = this->native_input_layouts.get(mapped_handle.resource_index);
        if (opt_native_resource.is_none()) {
            return;
        }
        opt_native_resource.unwrap_mut().Reset();

        this->handle_mapper.remove(handle);
    }

    foundation::Option<InputLayoutPool::NativeInputLayout&>
    InputLayoutPool::get_native_input_layout(const types::HandleId handle) noexcept {
        return this->handle_mapper.get(handle).and_then(
            [this](const decltype(handle_mapper)::ValueType& mapped_handle) {
                return this->native_input_layouts.get(mapped_handle.resource_index);
            });
    }

    foundation::Option<const InputLayoutPool::NativeInputLayout&>
    InputLayoutPool::get_native_input_layout(const types::HandleId handle) const noexcept {
        return this->handle_mapper.get(handle).and_then(
            [this](const decltype(handle_mapper)::ValueType& mapped_handle) {
                return this->native_input_layouts.get(mapped_handle.resource_index);
            });
    }
} // namespace enishi::renderer::directx