#include "bone_node.h"
#include "bone_node_tree.h"
#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

namespace enishi::platform_impl {
    void BoneNode::set_animation_translation(glm::vec3&& translation) noexcept {
        this->animation.transform.translation = std::move(translation);
    }

    void BoneNode::set_animation_translation(const glm::mat4& translation) noexcept {
        this->animation.transform.translation = glm::vec3(translation[3]);
    }

    void BoneNode::set_animation_rotation(glm::quat&& rotation) noexcept {
        this->animation.transform.rotation = std::move(rotation);
    }

    void BoneNode::set_animation_rotation(const glm::quat& rotation) noexcept {
        this->animation.transform.rotation = rotation;
    }

    void BoneNode::set_animation_scale(glm::vec3&& scale) noexcept {
        this->animation.transform.scale = std::move(scale);
    }

    void BoneNode::set_animation_scale(const glm::vec3& scale) noexcept {
        this->animation.transform.scale = scale;
    }

    glm::vec3& BoneNode::get_animation_translation(void) noexcept {
        return this->animation.transform.translation;
    }

    const glm::vec3& BoneNode::get_animation_translation(void) const noexcept {
        return this->animation.transform.translation;
    }

    glm::quat& BoneNode::get_animation_rotation(void) noexcept {
        return this->animation.transform.rotation;
    }

    const glm::quat& BoneNode::get_animation_rotation(void) const noexcept {
        return this->animation.transform.rotation;
    }

    glm::vec3& BoneNode::get_animation_scale(void) noexcept {
        return this->animation.transform.scale;
    }

    const glm::vec3& BoneNode::get_animation_scale(void) const noexcept {
        return this->animation.transform.scale;
    }

    glm::mat4 BoneNode::get_animation_local_transform(void) const noexcept {
        const auto& local_transform = this->animation.transform;
        const glm::mat4 t = glm::translate(glm::mat4{1.0f}, local_transform.translation);
        const glm::mat4 r = glm::mat4_cast(local_transform.rotation);
        const glm::mat4 s = glm::scale(glm::mat4{1.0f}, local_transform.scale);
        return t * r * s;
    }

    glm::mat4 BoneNode::get_animation_global_transform(void) const noexcept {
        // this->bind.global * this->get_animation_local_transform()
        return this->animation.global;
    }

    void BoneNode::set_physics_local(glm::mat4&& local) noexcept {
        this->physics.local = std::move(local);
    }

    void BoneNode::set_physics_local(const glm::mat4& local) noexcept {
        this->physics.local = local;
    }

    void BoneNode::set_physics_global(glm::mat4&& global) noexcept {
        this->physics.global = std::move(global);
    }

    void BoneNode::set_physics_global(const glm::mat4& global) noexcept {
        this->physics.global = global;
    }

    glm::mat4& BoneNode::get_physics_local(void) noexcept {
        return this->physics.local;
    }

    const glm::mat4& BoneNode::get_physics_local(void) const noexcept {
        return this->physics.local;
    }

    glm::mat4& BoneNode::get_physics_global(void) noexcept {
        return this->physics.global;
    }

    const glm::mat4& BoneNode::get_physics_global(void) const noexcept {
        return this->physics.global;
    }

    const glm::mat4& BoneNode::get_bind_local(void) const noexcept {
        return this->bind.local;
    }
    const glm::mat4& BoneNode::get_bind_global(void) const noexcept {
        return this->bind.global;
    }
    const glm::mat4& BoneNode::get_bind_global_inverse(void) const noexcept {
        return this->bind.global_inverse;
    }

    void BoneNode::update_animation_global(void) noexcept {
        auto tree = this->node_tree.lock();
        if (!bool(tree)) {
            return;
        }

        if (auto parent = tree->get_node(this->node.parent)) {
            // global = parent_global * local
            this->set_global(MatrixOperator::Assign,
                types::BoneKind::Animation,
                parent->get_animation_global_transform() * this->get_animation_local_transform());
        } else {
            // global = local
            this->write_local_to_global(
                types::BoneKind::Animation, types::BoneKind::Animation, MatrixOperator::Assign);
        }

        this->update_animation_children_global();
    }
    void BoneNode::update_animation_children_global(void) noexcept {
        auto tree = this->node_tree.lock();
        if (!bool(tree)) {
            return;
        }
        for (const auto& child : this->node.children) {
            if (auto node = tree->get_node(child)) {
                node->update_animation_global();
            }
        }
    }

    void BoneNode::write_local(
        const types::BoneKind from, const types::BoneKind to, const MatrixOperator op) noexcept {
        auto&& opt_mat = this->get_local(from);
        if (opt_mat.is_none()) {
            return;
        }

        this->set_local(op, to, std::move(opt_mat.unwrap_mut()));
    }
    void BoneNode::write_global(
        const types::BoneKind from, const types::BoneKind to, const MatrixOperator op) noexcept {
        auto&& opt_mat = this->get_global(from);
        if (opt_mat.is_none()) {
            return;
        }

        this->set_global(op, to, std::move(opt_mat.unwrap_mut()));
    }
    void BoneNode::write_local_to_global(
        const types::BoneKind from, const types::BoneKind to, const MatrixOperator op) noexcept {
        auto&& opt_mat = this->get_local(from);
        if (opt_mat.is_none()) {
            return;
        }

        this->set_global(op, to, std::move(opt_mat).unwrap_mut());
    }
    void BoneNode::write_global_to_local(
        const types::BoneKind from, const types::BoneKind to, const MatrixOperator op) noexcept {
        auto&& opt_mat = this->get_global(from);
        if (opt_mat.is_none()) {
            return;
        }

        this->set_local(op, to, std::move(opt_mat).unwrap_mut());
    }

    const types::BindBone& BoneNode::get_bind_bone(void) const noexcept {
        return this->bind;
    }
    const types::AnimationBone& BoneNode::get_animation_bone(void) const noexcept {
        return this->animation;
    }
    const types::PhysicsBone& BoneNode::get_physics_bone(void) const noexcept {
        return this->physics;
    }
    const types::SkinningBone& BoneNode::get_skinning_bone(void) const noexcept {
        return this->skinning;
    }

    foundation::Option<glm::mat4> BoneNode::get_local(const types::BoneKind kind) const {
        switch (kind) {
            case types::BoneKind::Animation:
                return this->get_animation_local_transform();
            case types::BoneKind::Bind:
                return this->bind.local;
            case types::BoneKind::Cache:
                return this->cache.local;
            case types::BoneKind::Physics:
                return this->physics.local;
            default:
                break;
        }
        return {};
    }
    foundation::Option<glm::mat4> BoneNode::get_global(const types::BoneKind kind) const {
        switch (kind) {
            case types::BoneKind::Animation:
                return this->get_animation_global_transform();
            case types::BoneKind::Bind:
                return this->bind.global;
            case types::BoneKind::Cache:
                return this->cache.global;
            case types::BoneKind::Physics:
                return this->physics.global;
            case types::BoneKind::Skinning:
                return this->skinning.global;
            default:
                break;
        }

        return {};
    }
    void BoneNode::set_local(const MatrixOperator op, const types::BoneKind kind, glm::mat4&& mat) {
        switch (kind) {
            case types::BoneKind::Cache: {
                switch (op) {
                    case MatrixOperator::Assign: {
                        this->cache.local = mat;
                    } break;
                    case MatrixOperator::LeftMulAssign: {
                        this->cache.local = mat * this->cache.local;
                    } break;
                    case MatrixOperator::MulAssign: {
                        this->cache.local *= mat;
                    } break;
                }
            } break;
            case types::BoneKind::Physics: {
                switch (op) {
                    case MatrixOperator::Assign: {
                        this->physics.local = mat;
                    } break;
                    case MatrixOperator::LeftMulAssign: {
                        this->physics.local = mat * this->physics.local;
                    } break;
                    case MatrixOperator::MulAssign: {
                        this->physics.local *= mat;
                    } break;
                }
            } break;
            default:
                break;
        }
    }
    void BoneNode::set_global(
        const MatrixOperator op, const types::BoneKind kind, glm::mat4&& mat) {
        switch (kind) {
            case types::BoneKind::Animation: {
                switch (op) {
                    case MatrixOperator::Assign: {
                        this->animation.global = mat;
                    } break;
                    case MatrixOperator::LeftMulAssign: {
                        this->animation.global = mat * this->animation.global;
                    } break;
                    case MatrixOperator::MulAssign: {
                        this->animation.global *= mat;
                    } break;
                }
            } break;
            case types::BoneKind::Cache: {
                switch (op) {
                    case MatrixOperator::Assign: {
                        this->cache.global = mat;
                    } break;
                    case MatrixOperator::LeftMulAssign: {
                        this->cache.global = mat * this->cache.global;
                    } break;
                    case MatrixOperator::MulAssign: {
                        this->cache.global *= mat;
                    } break;
                }
            } break;
            case types::BoneKind::Physics: {
                switch (op) {
                    case MatrixOperator::Assign: {
                        this->physics.global = mat;
                    } break;
                    case MatrixOperator::LeftMulAssign: {
                        this->physics.global = mat * this->physics.global;
                    } break;
                    case MatrixOperator::MulAssign: {
                        this->physics.global *= mat;
                    } break;
                }
            } break;
            case types::BoneKind::Skinning: {
                switch (op) {
                    case MatrixOperator::Assign: {
                        this->skinning.global = mat;
                    } break;
                    case MatrixOperator::LeftMulAssign: {
                        this->skinning.global = mat * this->skinning.global;
                    } break;
                    case MatrixOperator::MulAssign: {
                        this->skinning.global *= mat;
                    } break;
                }
            } break;
            default:
                break;
        }
    }
} // namespace enishi::platform_impl