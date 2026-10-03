#pragma once
#include <BulletDynamics/Dynamics/btRigidBody.h>
#include <engine_types/physics/rigid_body/rigid_body_impulse.h>

namespace enishi::physics::bullet3 {
    void apply_rigid_body_impulse(btRigidBody& body, const types::RigidBodyImpulse& impulse);
}
