#pragma once
#include <platform/renderer/interface_updater_accessor.h>
#include <platform/renderer/updater/interface_resource_updater.h>
#include <renderer/common/handle_mapper.h>
#include <renderer/common/resource_pool.h>

namespace enishi::renderer {
    class UpdaterPool : public platform::IUpdaterAccessor {
      private:
        struct UpdaterHandle {
            std::size_t index;
        };

      private:
        HandleMapper<UpdaterHandle> handle_mapper;
        ResourcePool<std::shared_ptr<platform::IResourceUpdater>> updater;

      public:
    };
} // namespace enishi::renderer