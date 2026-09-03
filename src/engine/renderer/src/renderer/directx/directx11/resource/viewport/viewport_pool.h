#pragma once
#include "interface_native_viewport_accessor.h"
#include <engine_types/handle/handle_mapper.h>
#include <engine_types/handle/handle_type.h>
#include <foundation/pool/resource_pool.h>
#include <renderer/errors/errors.h>
#include <vector>

namespace enishi::renderer::directx {
    class ViewportPool : public INativeViewportAccessor {
      private:
        struct ViewportHandle {
            std::size_t index;
        };

      private:
        types::ResourceMapper<ViewportHandle> handle_mapper;
        foundation::ResourcePool<NativeViewport> native_viewports;

      public:
        std::tuple<types::HandleId, NativeViewport&> make_native_viewport(void) noexcept override;
        void remove_native_viewport(const types::HandleId handle) noexcept override;
        foundation::Option<NativeViewport&> get_native_viewport(
            const types::HandleId handle) noexcept override;
        foundation::Option<const NativeViewport&> get_native_viewport(
            const types::HandleId handle) const noexcept override;
        std::span<const NativeViewport> get_native_viewports(void) const noexcept override;
    };
} // namespace enishi::renderer::directx