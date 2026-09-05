#pragma once
#include <engine_types/assets/model/addons/bone.h>
#include <foundation/option/option.h>
#include <memory>
#include <platform/animation/interface_animation_bone_view.h>
#include <platform/animation/interface_bone_updater.h>
#include <platform/asset/bone/interface_bind_bone_view.h>
#include <platform/bone_ststem/interface_bone_view.h>
#include <platform/bone_ststem/interface_bone_writer.h>
#include <platform/physics/bone/interface_physics_bone_view.h>

namespace enishi::platform_impl {
    class BoneNodeTree;
    class BoneNode : public platform::IAnimationBoneView,
                     public platform::IPhysicsBoneView,
                     public platform::IBindBoneView,
                     public platform::IBoneUpdater,
                     public platform::IBoneWriter,
                     public platform::IBoneView {
      private:
        std::weak_ptr<BoneNodeTree> node_tree;
        types::CacheBone cache;
        types::AnimationBone animation;
        types::PhysicsBone physics;
        types::BindBone bind;
        types::SkinningBone skinning;
        types::BoneNode node;

      public:
        explicit BoneNode(std::weak_ptr<BoneNodeTree> node_tree, const types::BindBone& bind_bone);

      public:
        void set_animation_translation(glm::vec3&& translation) noexcept override;
        void set_animation_translation(const glm::mat4& translation) noexcept override;
        void set_animation_rotation(glm::quat&& rotation) noexcept override;
        void set_animation_rotation(const glm::quat& rotation) noexcept override;
        void set_animation_scale(glm::vec3&& scale) noexcept override;
        void set_animation_scale(const glm::vec3& scale) noexcept override;
        glm::vec3& get_animation_translation(void) noexcept override;
        const glm::vec3& get_animation_translation(void) const noexcept override;
        glm::quat& get_animation_rotation(void) noexcept override;
        const glm::quat& get_animation_rotation(void) const noexcept override;
        glm::vec3& get_animation_scale(void) noexcept override;
        const glm::vec3& get_animation_scale(void) const noexcept override;
        glm::mat4 get_animation_local_transform(void) const noexcept override;
        glm::mat4 get_animation_global_transform(void) const noexcept override;

      public:
        void set_physics_local(glm::mat4&& local) noexcept override;
        void set_physics_local(const glm::mat4& local) noexcept override;
        void set_physics_global(glm::mat4&& global) noexcept override;
        void set_physics_global(const glm::mat4& global) noexcept override;
        glm::mat4& get_physics_local(void) noexcept override;
        const glm::mat4& get_physics_local(void) const noexcept override;
        glm::mat4& get_physics_global(void) noexcept override;
        const glm::mat4& get_physics_global(void) const noexcept override;

      public:
        const glm::mat4& get_bind_local(void) const noexcept override;
        const glm::mat4& get_bind_global(void) const noexcept override;
        const glm::mat4& get_bind_global_inverse(void) const noexcept override;

      public:
        void update_animation_global(void) noexcept override;
        void update_animation_children_global(void) noexcept override;

      public:
        void write_local(const types::BoneKind from,
            const types::BoneKind to,
            const MatrixOperator op) noexcept override;
        void write_global(const types::BoneKind from,
            const types::BoneKind to,
            const MatrixOperator op) noexcept override;
        void write_local_to_global(const types::BoneKind from,
            const types::BoneKind to,
            const MatrixOperator op) noexcept override;
        void write_global_to_local(const types::BoneKind from,
            const types::BoneKind to,
            const MatrixOperator op) noexcept override;

      public:
        const types::BindBone& get_bind_bone(void) const noexcept override;
        const types::AnimationBone& get_animation_bone(void) const noexcept override;
        const types::PhysicsBone& get_physics_bone(void) const noexcept override;
        const types::SkinningBone& get_skinning_bone(void) const noexcept override;

      private:
        foundation::Option<glm::mat4> get_local(const types::BoneKind kind) const;
        foundation::Option<glm::mat4> get_global(const types::BoneKind kind) const;
        void set_local(const MatrixOperator op, const types::BoneKind kind, glm::mat4&& mat);
        void set_global(const MatrixOperator op, const types::BoneKind kind, glm::mat4&& mat);
    };
} // namespace enishi::platform_impl