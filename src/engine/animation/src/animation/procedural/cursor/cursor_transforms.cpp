#include "cursor_transforms.h"
#include <algorithm>
#include <cmath>
#include <glm/gtc/matrix_transform.hpp>

namespace enishi::animation {
    void CursorTransforms::update_globals(CursorTransformCache& transforms,
        const component::ModelComponent& model,
        const component::AnimationComponent& animation) {
        for (const auto index : transforms.traversal) {
            const auto& local = animation.animation[index];
            const auto parent = model.bone_node[index].parent;
            const auto& cached = transforms.cached_local[index];
            const bool changed =
                !transforms.valid || local.position != cached.position ||
                local.rotation != cached.rotation || local.scale != cached.scale ||
                transforms.external[index] != transforms.cached_external[index] ||
                (parent < transforms.globals.size() && transforms.changed_globals[parent]);
            transforms.changed_globals[index] = changed;
            if (!changed) {
                continue;
            }
            transforms.cached_local[index] = local;
            transforms.cached_external[index] = transforms.external[index];
            const auto parent_global =
                parent < transforms.globals.size() ? transforms.globals[parent] : glm::mat4(1);
            // G = external * G_parent * T * R * S, matching the skinning hierarchy.
            transforms.globals[index] = transforms.external[index] * parent_global *
                                        glm::translate(glm::mat4(1), local.position) *
                                        glm::mat4_cast(local.rotation) *
                                        glm::scale(glm::mat4(1), local.scale);
        }
        transforms.valid = true;
    }
} // namespace enishi::animation
