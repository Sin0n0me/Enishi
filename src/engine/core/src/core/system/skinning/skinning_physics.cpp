#include "skinning_system.h"

namespace enishi::core {
    void SkinningSystem::import_physics_pose(ModelBones& bones) const noexcept {
        if (bones.physics_cache == nullptr || bones.ik_cache == nullptr) {
            return;
        }
        const auto nodes = bones.ik_updater->bone_nodes();
        const auto update =
            [&](auto&& self, types::BoneIndex index, const glm::mat4& parent) -> void {
            auto* view = bones.ik_cache->at(index);
            const auto global = bones.physics_driven[index]
                                    ? bones.physics_cache->at(index)->get_physics_global()
                                    : bones.external_transforms[index] * parent *
                                          bones.ik_base_local[index] *
                                          glm::mat4_cast(view->get_ik_rotation());
            view->set_ik_global_transform(global);
            for (const auto child : nodes[index].children) {
                self(self, child, global);
            }
        };
        for (types::BoneIndex index = 0; index < nodes.size(); ++index) {
            if (!nodes[index].has_parent()) {
                update(update, index, glm::mat4(1));
            }
        }
        // Rebase only simulated bones. Descendants keep their authored local pose,
        // and after-physics IK can operate on the resulting hierarchy normally.
        for (types::BoneIndex index = 0; index < nodes.size(); ++index) {
            if (!bones.physics_driven[index]) {
                continue;
            }
            const auto parent =
                nodes[index].has_parent()
                    ? bones.ik_cache->at(nodes[index].parent)->get_ik_global_transform()
                    : glm::mat4(1);
            auto* view = bones.ik_cache->at(index);
            bones.ik_base_local[index] = glm::inverse(bones.external_transforms[index] * parent) *
                                         view->get_ik_global_transform();
            view->set_ik_rotation(glm::quat(1, 0, 0, 0));
        }
    }

    void SkinningSystem::update_after_physics(void) {
        for (auto [entity, animation, model, skinning] :
            this->registry->view<component::AnimationComponent,
                component::ModelComponent,
                component::SkinningComponent>()) {
            const auto found = this->model_bones.find(entity);
            if (found == this->model_bones.end() || !found->second->pending_after_physics) {
                continue;
            }
            auto& bones = *found->second;
            bones.pending_after_physics = false;
            if (this->physics_engine != nullptr &&
                this->physics_engine->get_world()->get_config_reader()->can_update()) {
                this->import_physics_pose(bones);
            }
            this->evaluate_bone_phase(
                bones, this->registry->get<component::IKComponent>(entity), true);
            this->write_skinning_matrices(animation, model, skinning);
        }
    }
} // namespace enishi::core
