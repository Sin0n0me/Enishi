#include "bone_inheritance.h"
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/matrix_decompose.hpp>

namespace enishi::core {
    namespace {
        glm::quat rotation_of(const glm::mat4& matrix) {
            glm::vec3 scale;
            glm::quat rotation;
            glm::vec3 translation;
            glm::vec3 skew;
            glm::vec4 perspective;
            if (!glm::decompose(matrix, scale, rotation, translation, skew, perspective)) {
                return glm::quat(1, 0, 0, 0);
            }
            return glm::normalize(rotation);
        }
    } // namespace

    void apply_bone_inheritance(ModelBones& bones, types::BoneIndex index) {
        if (!(index < bones.constraints.size())) {
            return;
        }
        const auto& constraint = bones.constraints[index];
        if (!(constraint.source < bones.ik_base_local.size()) ||
            (constraint.rotation_weight == 0.0f && constraint.translation_weight == 0.0f)) {
            return;
        }
        const auto source = constraint.source;
        const auto& source_local = bones.ik_base_local[source];
        const auto source_bind = bones.bind_cache->at(source)->get_bind_local();
        auto inherited_rotation = rotation_of(source_local);
        if (!constraint.local_space) {
            inherited_rotation = glm::inverse(rotation_of(source_bind)) * inherited_rotation;
        }
        inherited_rotation *= bones.ik_cache->at(source)->get_ik_rotation();
        const auto inherited_translation = glm::vec3(source_local[3] - source_bind[3]);
        auto& local = bones.ik_base_local[index];
        const auto bind_rotation = rotation_of(bones.bind_cache->at(index)->get_bind_local());
        const auto weighted = glm::slerp(
            glm::quat(1, 0, 0, 0), glm::normalize(inherited_rotation), constraint.rotation_weight);
        // Retain the destination bind frame and scale. Keep IK corrections separate
        // so a later inheritor observes the source's current IK correction.
        const auto rotation = bind_rotation * weighted * glm::inverse(bind_rotation);
        const auto translation =
            glm::vec3(local[3]) + inherited_translation * constraint.translation_weight;
        local = glm::mat4_cast(glm::normalize(rotation)) * local;
        local[3] = glm::vec4(translation, 1.0f);
        bones.ik_updater->update_global(index);
    }
} // namespace enishi::core
