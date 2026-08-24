#pragma once
#include "updater/interface_resource_updater.h"
#include <engine_types/handle/renderer/render_handle.h>
#include <foundation/option/option.h>
#include <foundation/str/str.h>
#include <memory>

namespace enishi::platform {
    class IUpdaterAccessor {
      public:
        using ResourceUpdater = std::shared_ptr<IResourceUpdater>;

      public:
        virtual ~IUpdaterAccessor(void) noexcept = default;

        [[nodiscard]] virtual RenderResult<types::RenderHandle> get_updater(
            const foundation::UTF8& name) = 0;
    };
} // namespace enishi::platform