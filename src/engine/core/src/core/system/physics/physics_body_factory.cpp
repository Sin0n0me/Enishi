#include "physics_body_factory.h"
#include <foundation/log/logger.h>

namespace enishi::core {
    void PhysicsBodyFactory::build(platform::IPhysicsWorld& world,
        const component::PhysicsBodiesComponent& bodies,
        const std::shared_ptr<skinning_system::PhysicsBonesCache>& physics_cache,
        std::shared_ptr<platform::IBoneUpdater> updater) noexcept {
        if (bodies.rigid_bodies.empty()) {
            return;
        }

        auto object_result = world.add_object();
        if (object_result.is_err()) {
            foundation::Logger::warning(object_result.unwrap_err().get_message());
            return;
        }
        const auto& object_handle = object_result.unwrap();

        // 剛体はModelData上のインデックス順に追加する
        // (ジョイントのrigid_body_a/bはこの順序(=剛体配列のインデックス)を参照しているため)
        for (const auto& rigid_body : bodies.rigid_bodies) {
            const auto opt_view = physics_cache->get_shared(rigid_body.relate_bone_index);
            if (opt_view.is_none()) {
                foundation::Logger::warning(std::format(
                    "剛体に対応するボーンが見つかりません. rigid_body: {}, bone_index: {}",
                    rigid_body.name,
                    rigid_body.relate_bone_index));
                continue;
            }

            auto result = world.add_rigid_body(
                object_handle, rigid_body, physics_cache, updater, opt_view.unwrap());
            if (result.is_err()) {
                foundation::Logger::warning(result.unwrap_err().get_message());
            }
        }

        for (const auto& joint : bodies.joints) {
            auto result = world.add_joint(object_handle, joint);
            if (result.is_err()) {
                foundation::Logger::warning(result.unwrap_err().get_message());
            }
        }
    }
} // namespace enishi::core