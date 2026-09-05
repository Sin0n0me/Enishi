#include "animation_player.h"
#include "ik/ik_solver.h"

namespace enishi::core {
    void AnimationPlayer::update_local(
        component::AnimationComponent& animation, const types::BoneIndex index) {
        auto& bone = animation.bone_buffer[index];

        // 回転の合成
        glm::quat rotate = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
        for (const auto order : animation.multiply_order) {
            const auto& iter = animation.order_map.find(order);
            if (iter == animation.order_map.end()) {
                continue;
            }
            const auto index = iter->second;
            rotate = bone.rotations[index] * rotate;
        }

        // 列優先なので M = T * R * S
        glm::mat4 model_matrix = glm::mat4(1.0f);
        model_matrix = glm::translate(model_matrix, bone.position);
        model_matrix = model_matrix * glm::mat4_cast(rotate);
        model_matrix = glm::scale(model_matrix, bone.scale);

        bone.local = model_matrix;
    }

    void AnimationPlayer::update_global(component::AnimationComponent& animation,
        const component::ModelComponent& model,
        const types::BoneIndex index) {
        const auto& bone_node = model.bone_node[index];
        auto& bone = animation.bone_buffer[index];
        if (bone_node.has_parent()) {
            const auto& parent_bone = animation.bone_buffer[bone_node.parent];
            bone.global = parent_bone.global * bone.local;
        } else {
            bone.global = bone.local;
        }

        for (const auto& child : bone_node.children) {
            AnimationPlayer::update_global(animation, model, child);
        }
    }

    void AnimationPlayer::update_children_global(component::AnimationComponent& animation,
        const component::ModelComponent& model,
        const types::BoneIndex index) {
        const auto& bone_node = model.bone_node[index];
        for (const auto& child : bone_node.children) {
            AnimationPlayer::update_global(animation, model, child);
        }
    }

    void AnimationPlayer::global_to_bone_matrices(
        component::AnimationComponent& animation, const types::BoneIndex index) {
        animation.bone_matrices[index] = animation.bone_buffer[index].global;
    }

    void AnimationPlayer::bone_matrices_to_global(
        component::AnimationComponent& animation, const types::BoneIndex index) {
        animation.bone_buffer[index].global = animation.bone_matrices[index];
    }

    void AnimationPlayer::apply_animation(component::AnimationComponent& animation,
        const component::ModelComponent& model,
        const types::BoneIndex index) {
    }

    void AnimationPlayer::apply_ik(component::AnimationComponent& animation,
        const component::IKComponent& ik_component,
        const component::ModelComponent& model,
        const types::BoneIndex index) {
        IKSolver::apply_ik(animation, ik_component, model, index);
    }

    void AnimationPlayer::apply_physics(component::AnimationComponent& animation,
        const component::ModelComponent& model,
        const types::BoneIndex index) {
    }
} // namespace enishi::core