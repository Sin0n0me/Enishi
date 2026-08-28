#include "bone.h"

namespace enishi::core {
    glm::mat4& Bone::get_bone_local(void) noexcept {
        return this->bone.animation_bone.local;
    }
    glm::mat4& Bone::get_bone_global(void) noexcept {
        return this->bone.animation_bone.global;
    }
    const types::BindBone& Bone::get_bind_bone(void) const noexcept {
        return this->bone.bind_bone;
    }
    platform::IBoneUpdater* Bone::get_updater(void) noexcept {
        return this;
    }
    const platform::IBoneUpdater* Bone::get_updater(void) const noexcept {
        return this;
    }
    foundation::Option<platform::IBoneUpdater*> Bone::get_parent_updater(void) noexcept {
        auto opt_parent = this->get_parent_accessor();
        if (opt_parent.is_none()) {
            return {};
        }
        return opt_parent.unwrap_mut()->get_updater();
    }

    foundation::Option<const platform::IBoneUpdater*> Bone::get_parent_updater(
        void) const noexcept {
        auto opt_parent = this->get_parent_accessor();
        if (opt_parent.is_none()) {
            return {};
        }
        return opt_parent.unwrap()->get_updater();
    }
    foundation::Option<platform::IBoneAccessor*> Bone::get_parent_accessor(void) noexcept {
        auto list = this->bones_list.lock();
        if (!bool(list)) {
            return {};
        }
        return list->get_bone_accessor(this->bone.bone_node.parent);
    }
    foundation::Option<const platform::IBoneAccessor*> Bone::get_parent_accessor(
        void) const noexcept {
        const auto list = this->bones_list.lock();
        if (!bool(list)) {
            return {};
        }
        return {};
        // return list->get_bone_accessor(this->bone.bone_node.parent);
    }

    void Bone::set_bone_local(glm::mat4&& matrix) noexcept {
        this->bone.animation_bone.local = matrix;
    }
    void Bone::set_bone_local(const glm::mat4& matrix) noexcept {
        this->bone.animation_bone.local = matrix;
    }
    void Bone::set_bone_global(glm::mat4&& matrix) noexcept {
        this->bone.animation_bone.global = matrix;
    }
    void Bone::set_bone_global(const glm::mat4& matrix) noexcept {
        this->bone.animation_bone.global = matrix;
    }
    void Bone::update_local(void) noexcept {
        // 累積を合成
        /*
        const auto anim_translate = MMDMatrix::make_translation_from_vector(this->translate);
        const auto translate_matrix = this->bind_bone.local * anim_translate;
        const auto rotate = DirectX::XMQuaternionMultiply(this->ik_rotate, this->animation_rotate);
        const auto rotate_matrix = MMDMatrix::make_rotation_from_quaternion(rotate);

        this->local = MMDMatrix::make_transform_matrix(translate_matrix,
            rotate_matrix,
            MMDMatrix::make_identity_matrix() // MMDにスケールはない
        );
        */
    }
    void Bone::update_global(void) noexcept {
        const auto opt_parent = this->get_parent_accessor();
        if (opt_parent.is_some()) {
            this->bone.animation_bone.global =
                opt_parent.unwrap()->get_bone_global() * this->bone.animation_bone.local;
        } else {
            this->bone.animation_bone.global = this->bone.animation_bone.local;
        }

        this->update_children_global();
    }

    void Bone::update_children_global(void) noexcept {
        for (const auto& child : this->bone.bone_node.children) {
            const auto list = this->bones_list.lock();
            if (!bool(list)) {
                return;
            }
            auto opt_child_bone = list->get_bone_accessor(child);
            if (opt_child_bone.is_none()) {
                continue;
            }
            auto child_bone = opt_child_bone.unwrap_mut()->get_updater();
            child_bone->update_global();
        }
    }

    /*
    void Bone::on_update(void) {
    }

    types::OwnedRenderData& Bone::get_resource(void) {
        return types::OwnedRenderData{std::vector<int>{}};
    }
    */
} // namespace enishi::core