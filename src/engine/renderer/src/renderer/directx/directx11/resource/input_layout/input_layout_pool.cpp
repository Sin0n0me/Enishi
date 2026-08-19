#include "input_layout_pool.h"

namespace enishi::renderer::directx {
    std::tuple<std::size_t, InputLayoutPool::NativeInputLayout&> InputLayoutPool::make_native_input_layout(
        void) noexcept {
        return this->input_layouts.make();
    }

    void InputLayoutPool::remove_native_input_layout(const std::size_t index) noexcept {
    }

    foundation::Option<InputLayoutPool::NativeInputLayout&> InputLayoutPool::get_native_input_layout(
        const std::size_t index) noexcept {
        return this->input_layouts.get(index);
    }

    foundation::Option<const InputLayoutPool::NativeInputLayout&> InputLayoutPool::get_native_input_layout(
        const std::size_t index) const noexcept {
        return this->input_layouts.get(index);
    }
} // namespace enishi::renderer::directx