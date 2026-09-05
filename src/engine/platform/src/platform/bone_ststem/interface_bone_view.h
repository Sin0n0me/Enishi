#pragma once
#include <engine_types/assets/model/addons/bone.h>
#include <foundation/option/option.h>
#include <glm/glm.hpp>

namespace enishi::platform {
    class IBoneView {
      public:
        virtual ~IBoneView(void) noexcept = default;

        virtual const types::BindBone& get_bind_bone(void) const noexcept = 0;

        virtual const types::AnimationBone& get_animation_bone(void) const noexcept = 0;

        virtual const types::PhysicsBone& get_physics_bone(void) const noexcept = 0;

        virtual const types::SkinningBone& get_skinning_bone(void) const noexcept = 0;

        virtual foundation::Option<const IBoneView*> get_parent_view(void) const noexcept = 0;
    };
} // namespace enishi::platform