#pragma once
#include "../../errors/errors.h"
#include <assets_system/interface_asset_system.h>
#include <foundation/result/result.h>
#include <memory>
#include <platform/renderer/interface_render_pass.h>
#include <platform/renderer/interface_renderer.h>

namespace enishi::core {
    class IRenderPassConstructor {
      public:
        virtual ~IRenderPassConstructor(void) noexcept = default;

        [[nodiscard]] virtual foundation::Result<std::shared_ptr<platform::IRenderPass>,
            SystemError>
        make(platform::IRenderer* const renderer,
            assets_system::IAssetSystem* const asset_system) = 0;
    };
} // namespace enishi::core