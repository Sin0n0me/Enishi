#pragma once
#include "interface_resource_updater.h"
#include <engine_types/handle/renderer/render_handle.h>
#include <foundation/result/result.h>
#include <platform/errors/renderer_errors.h>

namespace enishi::platform {
    class IUnifromUpdater : public IResourceUpdater {
      public:
        virtual ~IUnifromUpdater(void) noexcept = default;
    };
} // namespace enishi::platform