#include "physics_body_factory.h"

namespace enishi::core {
    foundation::Result<std::vector<types::PhysicsHandle>, sub_system::PhysicsError>
    PhysicsBodyFactory::build(sub_system::IPhysicsWorld& world,
        const component::PhysicsBodiesComponent& bodies,
        const std::shared_ptr<skinning_system::PhysicsBonesCache>& physics_cache,
        std::shared_ptr<sub_system::IBoneUpdater> updater) noexcept {
        if (bodies.rigid_bodies.empty()) {
            return std::vector<types::PhysicsHandle>{};
        }
        if (physics_cache == nullptr || updater == nullptr) {
            return foundation::Error(sub_system::PhysicsError::MakeError,
                "Physics body construction requires a bone cache and updater");
        }
        for (const auto& body : bodies.rigid_bodies) {
            if (!(body.relate_bone_index < physics_cache->size())) {
                return foundation::Error(
                    sub_system::PhysicsError::MakeError, "Rigid body references a missing bone");
            }
        }
        for (const auto& joint : bodies.joints) {
            if (!(joint.rigid_body_a < bodies.rigid_bodies.size()) ||
                !(joint.rigid_body_b < bodies.rigid_bodies.size()) ||
                joint.rigid_body_a == joint.rigid_body_b) {
                return foundation::Error(
                    sub_system::PhysicsError::MakeError, "Invalid joint body references");
            }
        }

        auto object_result = world.add_object();
        if (object_result.is_err()) {
            return std::move(object_result)
                .unwrap_err()
                .add_message("Failed to create the model physics object");
        }
        const auto& object_handle = object_result.unwrap();
        std::vector<types::PhysicsHandle> handles;

        // 剛体はModelData上のインデックス順に追加する
        // (ジョイントのrigid_body_a/bはこの順序(=剛体配列のインデックス)を参照しているため)
        for (const auto& rigid_body : bodies.rigid_bodies) {
            const auto opt_view = physics_cache->get_shared(rigid_body.relate_bone_index);
            if (opt_view.is_none()) {
                return foundation::Error(
                    sub_system::PhysicsError::MakeError, "Rigid body bone view is missing");
            }

            auto result = world.add_rigid_body(
                object_handle, rigid_body, physics_cache, updater, opt_view.unwrap());
            if (result.is_err()) {
                return std::move(result).unwrap_err().add_message(
                    std::format("Failed to create model rigid body {}", handles.size()));
            }
            handles.push_back(result.unwrap());
        }

        for (const auto& joint : bodies.joints) {
            auto result = world.add_joint(object_handle, joint);
            if (result.is_err()) {
                return std::move(result).unwrap_err().add_message(
                    std::format("Failed to create joint between rigid bodies {} and {}",
                        joint.rigid_body_a,
                        joint.rigid_body_b));
            }
        }
        return handles;
    }
} // namespace enishi::core
