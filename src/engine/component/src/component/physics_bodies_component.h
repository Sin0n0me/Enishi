#pragma once
#include <engine_types/handle/physics/physics_handle.h>
#include <engine_types/physics/joint/physics_joint.h>
#include <engine_types/physics/rigid_body/physics_rigid_body.h>
#include <vector>

namespace enishi::component {
    // モデルが持つ剛体・ジョイントの定義
    struct PhysicsBodiesComponent {
        std::vector<types::PhysicsRigidBody> rigid_bodies;
        std::vector<types::PhysicsJoint> joints;
        std::vector<types::PhysicsHandle> handles;
        // Requested visibility and the last state successfully applied to the physics world.
        bool enabled = true;
        bool simulation_enabled = true;
    };
} // namespace enishi::component
