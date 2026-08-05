#pragma once
#include <component/animation_component.h>
#include <component/ik_component.h>
#include <component/model_component.h>
#include <component/physics_component.h>
#include <ecs/registory.h>
#include <engine_types/assets/model/addons/bone.h>
#include <memory>

namespace enishi::core {
    class AnimationPlayer {
      public:
        static void update_local(
            component::AnimationComponent& animation, const types::BoneIndex index);

        static void update_global(component::AnimationComponent& animation,
            const component::ModelComponent& model,
            const types::BoneIndex index);

        static void update_children_global(component::AnimationComponent& animation,
            const component::ModelComponent& model,
            const types::BoneIndex index);

        static void global_to_bone_matrices(
            component::AnimationComponent& animation, const types::BoneIndex index);

        static void bone_matrices_to_global(
            component::AnimationComponent& animation, const types::BoneIndex index);

        static void apply_animation(component::AnimationComponent& animation,
            const component::ModelComponent& model,
            const types::BoneIndex index);

        static void apply_ik(component::AnimationComponent& animation,
            const component::IKComponent& ik_component,
            const component::ModelComponent& model,
            const types::BoneIndex index);

        static void apply_physics(component::AnimationComponent& animation,
            const component::ModelComponent& model,
            const types::BoneIndex index);
    };
} // namespace enishi::core