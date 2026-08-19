#pragma once
#include "interface_native_input_layout_accessor.h"
#include <cstdint>
#include <d3d11.h>
#include <renderer/common/resource_pool.h>
#include <wrl/client.h>

namespace enishi::renderer::directx {
    class InputLayoutPool : public INativeInputLayoutAccessor {
      private:
        ResourcePool<NativeInputLayout> input_layouts;

      public:
        std::tuple<std::size_t, NativeInputLayout&> make_native_input_layout(void) noexcept override;
        void remove_native_input_layout(const std::size_t index) noexcept override;
        foundation::Option<NativeInputLayout&> get_native_input_layout(
            const std::size_t index) noexcept override;
        foundation::Option<const NativeInputLayout&> get_native_input_layout(
            const std::size_t index) const noexcept override;
    };
} // namespace enishi::renderer::directx