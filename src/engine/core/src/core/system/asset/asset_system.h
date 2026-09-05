#pragma once
#include "asset_manager.h"
#include <core/system/interface_system.h>
#include <memory>

namespace enishi::core {
    class AssetSystem : public ISystem {
      private:
        std::shared_ptr<AssetManager> asset_manager;

      public:
        explicit AssetSystem(void);

        std::shared_ptr<assets_system::IAssetSystem> get_asset_system(void);

      public:
        bool should_close(void) override;
        void pre_update(void) override;
        void update(const types::DeltaTime& delta_time) override;
        void post_update(void) override;
        void render(void) const override;
    };
} // namespace enishi::core