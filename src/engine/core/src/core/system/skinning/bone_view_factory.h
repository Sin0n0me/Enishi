#pragma once
#include <component/animation_component.h>
#include <component/ik_component.h>
#include <component/model_component.h>
#include <component/physics_component.h>
#include <engine_types/assets/model/addons/bone.h>
#include <memory>
#include <ranges>
#include <skinning_system/views/animation_bone_view.h>
#include <skinning_system/views/bind_bone_view.h>
#include <skinning_system/views/ik_bone_view.h>
#include <skinning_system/views/physics_bone_view.h>
#include <vector>

namespace enishi::core {
    /**
     * component(ECS)側のデータとskinning_system側のView実装を結びつける
     * ここだけがcomponentとskinning_systemの両方を知っていてよい場所とする
     */
    class BoneViewFactory final {
      public:
        template <typename T> using Views = std::vector<std::unique_ptr<T>>;

      public:
        BoneViewFactory(void) = delete;

        [[nodiscard]] static Views<skinning_system::AnimationBoneView> make_animation_view(
            component::AnimationComponent& animation) noexcept;

        [[nodiscard]] static Views<skinning_system::PhysicsBoneView> make_physics_view(
            component::PhysicsComponent& physics) noexcept;

        [[nodiscard]] static Views<skinning_system::IKBoneView> make_ik_view(
            component::IKComponent& ik) noexcept;

        [[nodiscard]] static Views<skinning_system::BindBoneView> make_bind_view(
            const component::ModelComponent& model) noexcept;

        template <typename T>
        [[nodiscard]] static std::vector<std::shared_ptr<T>> to_shared_views(Views<T>&& views) {
            return views | std::views::as_rvalue |
                   std::ranges::to<std::vector<std::shared_ptr<T>>>();
        }
    };
} // namespace enishi::core
