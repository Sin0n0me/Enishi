#pragma once
#include "interface_native_viewport_accessor.h"
#include <d3d11.h>
#include <engine_types/handle/handle_type.h>
#include <engine_types/renderer/description/image_view_description.h>
#include <foundation/option/option.h>
#include <foundation/result/result.h>
#include <memory>
#include <renderer/errors/errors.h>
#include <unordered_map>
#include <vector>
#include <wrl/client.h>

namespace enishi::renderer::directx {
    class ViewportPool : public INativeViewportAccessor {
      private:
        std::vector<Viewport> viewports;

      public:
        std::tuple<std::size_t, Viewport&> make_native_viewport(void) noexcept override;
        void remove_native_viewport(const std::size_t index) noexcept override;
        foundation::Option<Viewport&> get_native_viewport(const std::size_t index) noexcept override;
        foundation::Option<const Viewport&> get_native_viewport(
            const std::size_t index) const noexcept override;
        std::span<const Viewport> get_native_viewports(void) const noexcept override;
    };
} // namespace enishi::renderer::directx