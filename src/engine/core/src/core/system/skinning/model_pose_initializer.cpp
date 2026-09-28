#include "model_pose_initializer.h"
#include <algorithm>
#include <component/animation_component.h>
#include <component/ik_component.h>
#include <component/morph_component.h>
#include <component/skinning_component.h>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/matrix_decompose.hpp>

namespace enishi::core {
    foundation::Result<void, ModelPoseError> initialize_model_pose(
        ecs::Registry& registry, types::HandleId entity, const component::ModelComponent& model) {
        if (!model.morph_targets.targets.empty()) {
            component::MorphComponent morph;
            morph.weights.resize(model.morph_targets.targets.size());
            auto result = registry.insert(entity, std::move(morph));
            if (result.is_err()) {
                return result.propagation(ModelPoseError::RegistrationFailed);
            }
        }
        if (model.bone_node.empty()) {
            return {};
        }
        if (model.bind_bone.size() != model.bone_node.size()) {
            return foundation::Error(
                ModelPoseError::InvalidBindTransform, "Bone and bind counts differ");
        }
        component::AnimationComponent animation{};
        component::IKComponent ik;
        component::SkinningComponent skinning;
        for (const auto& bind : model.bind_bone) {
            component::AnimationBuffer pose;
            glm::vec3 skew;
            glm::vec4 perspective;
            if (!glm::decompose(
                    bind.local, pose.scale, pose.rotation, pose.position, skew, perspective)) {
                return foundation::Error(
                    ModelPoseError::InvalidBindTransform, "Invalid bind transform");
            }
            animation.animation.push_back(pose);
            animation.bind_pose.push_back(pose);
            animation.global.push_back(bind.global);
            ik.rotation.emplace_back(1.0f, 0.0f, 0.0f, 0.0f);
            ik.globals.push_back(bind.global);
            skinning.skinning_matrices.push_back(bind.global * bind.global_inverse);
        }
        ik.iks = model.iks;
        for (std::size_t index = 0; index < ik.iks.size(); ++index) {
            const auto& chain = std::get<types::CCDIK>(ik.iks[index].method);
            ik.ik_map.emplace(chain.ik_bone, static_cast<types::IkIndex>(index));
        }
        auto animation_result = registry.insert(entity, std::move(animation));
        if (animation_result.is_err()) {
            return animation_result.propagation(ModelPoseError::RegistrationFailed);
        }
        auto skinning_result = registry.insert(entity, std::move(skinning));
        if (skinning_result.is_err()) {
            return skinning_result.propagation(ModelPoseError::RegistrationFailed);
        }
        const auto& constraints = model.bone_constraints.constraints;
        const auto has_inheritance =
            std::any_of(constraints.begin(), constraints.end(), [](const auto& constraint) {
                return constraint.rotation_weight != 0.0f || constraint.translation_weight != 0.0f;
            });
        const auto& targets = model.morph_targets.targets;
        const auto has_bone_morph =
            std::any_of(targets.begin(), targets.end(), [](const auto& target) {
                return std::any_of(
                    target.offsets.begin(), target.offsets.end(), [](const auto& offset) {
                        return std::holds_alternative<types::BoneMorphOffset>(offset);
                    });
            });
        if (!ik.iks.empty() || has_inheritance || has_bone_morph) {
            auto ik_result = registry.insert(entity, std::move(ik));
            if (ik_result.is_err()) {
                return ik_result.propagation(ModelPoseError::RegistrationFailed);
            }
        }
        return {};
    }
} // namespace enishi::core
