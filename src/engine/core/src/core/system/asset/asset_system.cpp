#include "asset_system.h"

namespace enishi::core {
    AssetSystem::AssetSystem(void)
        : asset_manager(std::make_shared<AssetManager>()) {
    }
    std::shared_ptr<assets_system::IAssetSystem> AssetSystem::get_asset_system(void) {
        return this->asset_manager;
    }
    bool AssetSystem::should_close(void) {
        return false;
    }
    void AssetSystem::pre_update(void) {
        this->asset_manager->drain_completed_loads();
    }
    void AssetSystem::update(const types::DeltaTime& delta_time) {
    }
    void AssetSystem::post_update(void) {
    }
    void AssetSystem::render(void) const {
    }
} // namespace enishi::core