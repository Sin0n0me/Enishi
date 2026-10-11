#include "physics_bones_updater.h"

namespace enishi::skinning_system {
    void PhysicsBonesUpdater::update_local(const types::BoneIndex index) noexcept {
        if (this->physics_view->size() < index + 1) {
            return;
        }
        const auto nodes = this->physics_view->get_bone_nodes();
        if (nodes.size() < index + 1) {
            return;
        }
        const auto& bone_node = nodes[index];
        const auto& view = this->physics_view->at(index);

        // 物理状態の反映(ローカル空間)
        const auto& global = view->get_physics_global();
        if (bone_node.has_parent()) {
            const auto& parent = this->physics_view->at(bone_node.parent);
            const auto& parent_global = parent->get_physics_global();
            const auto& local = glm::inverse(parent_global) * global;

            view->set_physics_local(local);
        } else {
            view->set_physics_local(global);
        }
    }

    void PhysicsBonesUpdater::update_global(const types::BoneIndex index) noexcept {
        if (this->physics_view->size() < index + 1) {
            return;
        }
        const auto nodes = this->physics_view->get_bone_nodes();
        if (nodes.size() < index + 1) {
            return;
        }
        const auto& bone_node = nodes[index];
        const auto& view = this->physics_view->at(index);

        const auto& local = view->get_physics_local();
        if (bone_node.has_parent()) {
            // global = parent_global * local
            const auto& prent_node = this->physics_view->at(bone_node.parent);
            const auto& prent_global = prent_node->get_physics_global();
            view->set_physics_global(prent_global * local);
        } else {
            // global = local
            view->set_physics_global(local);
        }

        this->update_children_global(index);
    }

    void PhysicsBonesUpdater::update_children_global(const types::BoneIndex index) noexcept {
        const auto nodes = this->physics_view->get_bone_nodes();
        if (nodes.size() < index + 1) {
            return;
        }
        const auto& bone_node = nodes[index];
        for (const auto& child : bone_node.children) {
            this->update_global(child);
        }
    }
    void skinning_system::PhysicsBonesUpdater::update_global_form_roots(void) noexcept {
        const auto nodes = this->physics_view->get_bone_nodes();
        for (types::BoneIndex i = 0; i < nodes.size(); ++i) {
            if (!nodes[i].has_parent()) {
                this->update_global(i);
            }
        }
    }
} // namespace enishi::skinning_system