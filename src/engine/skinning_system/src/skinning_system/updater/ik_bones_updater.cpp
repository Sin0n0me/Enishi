#include "ik_bones_updater.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

namespace enishi::skinning_system {
    IKBonesUpdater::IKBonesUpdater(
        IKBoneCache& ik_view, const sub_system::IBindBoneViewList& bind_view) noexcept
        : ik_view(&ik_view)
        , bind_view(&bind_view) {
    }

    std::span<const types::BoneNode> IKBonesUpdater::bone_nodes(void) const noexcept {
        return this->ik_view->get_bone_nodes();
    }

    void IKBonesUpdater::update_local(const types::BoneIndex index) noexcept {
        // globalの計算のたびにbind位置+ik_rotationからその場で計算するため、保存する値はない
    }

    void IKBonesUpdater::update_global(const types::BoneIndex index) noexcept {
        if (this->bone_nodes().size() <= index) {
            return;
        }
        auto* const view = this->ik_view->at(index);
        const auto& bind = this->bind_view->at(index);

        // pivotはバインドポーズのローカル位置(不変)を使う
        const auto bind_local_translation = glm::vec3(bind->get_bind_local()[3]);
        const auto local = glm::translate(glm::mat4(1.0f), bind_local_translation) *
                           glm::mat4_cast(view->get_ik_rotation());

        const auto& bone_node = this->bone_nodes()[index];
        if (bone_node.has_parent()) {
            auto* const parent_view = this->ik_view->at(bone_node.parent);
            const auto parent_global = parent_view->get_ik_global_transform();
            view->set_ik_global_transform(parent_global * local);
        } else {
            view->set_ik_global_transform(local);
        }

        this->update_children_global(index);
    }

    void IKBonesUpdater::update_children_global(const types::BoneIndex index) noexcept {
        if (this->bone_nodes().size() + 1 < index) {
            return;
        }
        for (const auto& child : this->bone_nodes()[index].children) {
            this->update_global(child);
        }
    }

    void IKBonesUpdater::update_global_form_roots(void) noexcept {
        const auto size = this->bone_nodes().size();
        for (types::BoneIndex i = 0; i < size; ++i) {
            if (!this->bone_nodes()[i].has_parent()) {
                this->update_global(i);
            }
        }
    }
} // namespace enishi::skinning_system