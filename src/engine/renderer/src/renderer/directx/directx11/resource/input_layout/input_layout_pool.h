#pragma once
#include "interface_native_input_layout_accessor.h"
#include <engine_types/handle/handle_mapper.h>
#include <renderer/common/resource_pool.h>

namespace enishi::renderer::directx {
    class InputLayoutPool : public INativeInputLayoutAccessor {
      private:
        struct InputLayoutHandle {
            std::size_t resource_index;
            std::size_t interface_index;
        };

      private:
        types::ResourceMapper<InputLayoutHandle> handle_mapper;
        ResourcePool<NativeInputLayout> native_input_layouts;

      public:
        std::tuple<types::HandleId, NativeInputLayout&> make_native_input_layout(
            void) noexcept override;
        void remove_native_input_layout(const types::HandleId handle) noexcept override;
        foundation::Option<NativeInputLayout&> get_native_input_layout(
            const types::HandleId handle) noexcept override;
        foundation::Option<const NativeInputLayout&> get_native_input_layout(
            const types::HandleId handle) const noexcept override;
    };
} // namespace enishi::renderer::directx