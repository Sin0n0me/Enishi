#pragma once
#include <engine_types/handle/handle_mapper.h>
#include <foundation/pool/resource_pool.h>
#include <platform/renderer/updater/interface_resource_updater.h>
#include <platform/renderer/updater/interface_updater_accessor.h>

namespace enishi::renderer {
    class UpdaterPool : public platform::IUpdaterAccessor {
      private:
        struct UpdaterHandle {
            std::size_t index;
        };

      private:
        types::ResourceMapper<UpdaterHandle> handle_mapper;
        foundation::ResourcePool<std::shared_ptr<platform::IResourceUpdater>> updater;

      public:
        foundation::Option<ResourceUpdater&> get_updater(const types::HandleId handle) override;
    };
} // namespace enishi::renderer