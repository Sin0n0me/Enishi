#pragma once
#include "interface_resource_updater.h"
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

        [[nodiscard]] virtual foundation::Option<ResourceUpdater&> get_updater(
            const types::HandleId handle) = 0;
    };
} // namespace enishi::platform