#include "physics_bones_updater.h"

namespace enishi::skinning_system {
    PhysicsBonesUpdater::PhysicsBonesUpdater(PhysicsBonesCache& physics_view) noexcept
        : physics_view(&physics_view)
        , transforms(physics_view.size()) {
    }

    void PhysicsBonesUpdater::update_local(const types::BoneIndex index) noexcept {
        const auto nodes = this->physics_view->get_bone_nodes();
        if (!(index < this->transforms.size()) || !(index < nodes.size())) {
            return;
        }
        const auto& node = nodes[index];
        auto* view = this->physics_view->at(index);
        const auto& global = view->get_physics_global();
        const auto parent_global = node.has_parent()
                                       ? this->physics_view->at(node.parent)->get_physics_global()
                                       : glm::mat4(1);
        auto& cache = this->transforms[index];
        // Compare actual inputs, not frame IDs: animation and physics can change them in one frame.
        if (cache.local_valid && cache.local_source == global &&
            cache.parent_source == parent_global) {
            if (view->get_physics_local() != cache.local) {
                view->set_physics_local(cache.local);
            }
            return;
        }
        if (node.has_parent()) {
            auto& parent = this->transforms[node.parent];
            if (!parent.inverse_valid || parent.inverse_source != parent_global) {
                parent.inverse = glm::inverse(parent_global);
                parent.inverse_source = parent_global;
                parent.inverse_valid = true;
            }
            // L = inverse(G_parent) * G_child; siblings share the same cached inverse.
            cache.local = parent.inverse * global;
        } else {
            cache.local = global;
        }
        cache.local_source = global;
        cache.parent_source = parent_global;
        cache.local_valid = true;
        view->set_physics_local(cache.local);
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