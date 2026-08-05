#pragma once
#include "addons/bone.h"
#include "addons/ik.h"
#include "addons/morph.h"
#include "addons/physics_joint.h"
#include "addons/rigid_body.h"
#include <variant>
#include <vector>

// モデルに対して変化を加える場合はこの型を使用
namespace enishi::types {
    using AddonBones = std::vector<Bone>;
    using AddonIKs = std::vector<IK>;
    using AddonRigidBodies = std::vector<RigidBody>;
    using AddonPhysicsJoints = std::vector<PhysicsJoint>;

    using ModelAddon = std::variant<std::monostate,
        AddonBones,
        AddonIKs,
        AddonMorphs,
        AddonRigidBodies,
        AddonPhysicsJoints>;
} // namespace enishi::types