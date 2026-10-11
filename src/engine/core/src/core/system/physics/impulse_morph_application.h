#pragma once
#include <component/model_component.h>
#include <component/morph_component.h>
#include <component/physics_bodies_component.h>
#include <sub_system/physics/interface_physics_world.h>

namespace enishi::core {
    [[nodiscard]] foundation::Result<void, sub_system::PhysicsError> apply_impulse_morphs(
        sub_system::IPhysicsWorld& world,
        const component::ModelComponent& model,
        const component::MorphComponent& morph,
        const component::PhysicsBodiesComponent& bodies);
}
