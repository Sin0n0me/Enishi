#pragma once
#include "model_bones.h"
#include <component/model_component.h>
#include <component/physics_bodies_component.h>
#include <component/physics_component.h>
#include <foundation/option/option.h>
#include <sub_system/physics/interface_physics_engine.h>
namespace enishi::core {
    class SkinningPhysics final {
      public:
        static void build(ModelBones& bones,
            sub_system::IPhysicsEngine* physics_engine,
            const component::ModelComponent& model,
            foundation::Option<component::PhysicsComponent&> physics,
            foundation::Option<component::PhysicsBodiesComponent&> physics_bodies);
        static void import_pose(ModelBones& bones) noexcept;
        static void write_back_pose(ModelBones& bones) noexcept;
    };
} // namespace enishi::core
