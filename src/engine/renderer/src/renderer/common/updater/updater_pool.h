#pragma once
#include <engine_types/handle/handle_mapper.h>
#include <engine_types/handle/renderer/handles/resource_handles.h>
#include <platform/renderer/updater/interface_resource_updater.h>
#include <platform/renderer/updater/interface_updater_accessor.h>
#include <renderer/common/resource_pool.h>

namespace enishi::renderer {
    class UpdaterPool : public platform::IUpdaterAccessor {
      private:
        struct UpdaterHandle {
            std::size_t index;
        };

      private:
        types::ResourceMapper<UpdaterHandle> handle_mapper;
        ResourcePool<std::shared_ptr<platform::IResourceUpdater>> updater;

      public:
        foundation::Option<ResourceUpdater&> get_updater(const types::HandleId handle) override;
    };
} // namespace enishi::renderer