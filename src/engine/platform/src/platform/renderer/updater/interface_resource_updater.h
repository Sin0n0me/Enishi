#pragma once
#include <engine_types/renderer/render_data.h>
#include <platform/errors/renderer_errors.h>

namespace enishi::platform {
    class IResourceUpdater {
      public:
        virtual ~IResourceUpdater(void) noexcept = default;

        virtual void on_update(void) = 0;

        [[nodiscard]] virtual types::OwnedRenderData& get_resource(void) = 0;
    };
} // namespace enishi::platform