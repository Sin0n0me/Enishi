#include "bone_view_factory.h"

namespace enishi::core {
    BoneViewFactory::Views<skinning_system::AnimationBoneView> BoneViewFactory::make_animation_view(
        component::AnimationComponent& animation) noexcept {
        const auto size = animation.animation.size();
        if (size != animation.animation.size()) {
            return {};
        }

        BoneViewFactory::Views<skinning_system::AnimationBoneView> views;
        views.reserve(size);
        for (std::size_t index = 0; index < size; ++index) {
            auto& bone = animation.animation[index];
            views.emplace_back(std::make_unique<skinning_system::AnimationBoneView>(
                bone.position, bone.rotation, bone.scale, animation.global[index]));
        }

        return views;
    }

    BoneViewFactory::Views<skinning_system::PhysicsBoneView> BoneViewFactory::make_physics_view(
        component::PhysicsComponent& physics) noexcept {
        const auto size = physics.local.size();
        if (size != physics.global.size()) {
            return {};
        }

        BoneViewFactory::Views<skinning_system::PhysicsBoneView> views;
        views.reserve(size);
        for (std::size_t index = 0; index < size; ++index) {
            views.emplace_back(std::make_unique<skinning_system::PhysicsBoneView>(
                physics.local[index], physics.global[index]));
        }

        return views;
    }

    BoneViewFactory::Views<skinning_system::IKBoneView> BoneViewFactory::make_ik_view(
        component::IKComponent& ik) noexcept {
        const auto size = ik.rotation.size();
        if (size != ik.globals.size()) {
            return {};
        }

        BoneViewFactory::Views<skinning_system::IKBoneView> views;
        views.reserve(size);
        for (std::size_t index = 0; index < size; ++index) {
            views.emplace_back(std::make_unique<skinning_system::IKBoneView>(
                ik.rotation[index], ik.globals[index]));
        }
    }

    BoneViewFactory::Views<skinning_system::BindBoneView> BoneViewFactory::make_bind_view(
        const component::ModelComponent& model) noexcept {
        const auto size = model.bind_bone.size();
        if (size != model.bone_node.size()) {
            return {};
        }

        BoneViewFactory::Views<skinning_system::BindBoneView> views;
        views.reserve(size);
        for (std::size_t index = 0; index < size; ++index) {
            const auto& bind_bone = model.bind_bone[index];
            views.emplace_back(std::make_unique<skinning_system::BindBoneView>(
                bind_bone.local, bind_bone.global, bind_bone.global_inverse));
        }

        return views;
    }
} // namespace enishi::core