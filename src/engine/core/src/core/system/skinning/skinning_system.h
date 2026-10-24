#pragma once
#include <debug/playback/playback_control.h>
#include "model_bones.h"
#include <array>
#include <component/animation_component.h>
#include <component/ik_component.h>
#include <component/model_component.h>
#include <component/physics_bodies_component.h>
#include <component/physics_component.h>
#include <component/skinning_component.h>
#include <core/system/interface_system.h>
#include <ecs/registry.h>
#include <engine_types/skinning/skinning_command.h>
#include <memory>
#include <span>
#include <sub_system/camera/interface_camera_controller.h>
#include <sub_system/physics/interface_physics_engine.h>
#include <unordered_map>
#include <variant>

namespace enishi::platform {
    class IWindow;
}

namespace enishi::core {
    /**
     * モデルが持つ各ボーンデータを適切な順序で計算し
     * 最終的にスキニング用の行列を作成する
     */
    class SkinningSystem final : public ISystem {
      private:
        using AddonComponent = std::variant<std::monostate,
            component::IKComponent*,
            component::PhysicsComponent*,
            component::PhysicsBodiesComponent*>;

      private:
        static constexpr std::array<types::SkinningCommand, 3> DEFAULT_ORDER = {
            types::SkinningCommand::Animation,
            types::SkinningCommand::IK,
            types::SkinningCommand::WriteBackPhysicsSimulate,
        };

        std::shared_ptr<ecs::Registry> registry;
        std::shared_ptr<debug::PlaybackControl> debug_playback;
        std::shared_ptr<sub_system::IPhysicsEngine> physics_engine;
        std::weak_ptr<platform::IWindow> cursor_window;
        std::shared_ptr<sub_system::ICameraController> camera;
        animation::CursorPoseConfig cursor_config;
        animation::BoneNameDictionary bone_names;
        std::unordered_map<types::HandleId, std::unique_ptr<ModelBones>> model_bones;

      public:
        explicit SkinningSystem(std::shared_ptr<ecs::Registry> registry,
            std::shared_ptr<sub_system::IPhysicsEngine> physics_engine) noexcept;

        void set_debug_playback(std::shared_ptr<debug::PlaybackControl> playback);
        void pre_update(void) override;
        void update(const types::DeltaTime& delta_time) override;
        void update_after_physics(void);
        void set_cursor_window(const std::shared_ptr<platform::IWindow>& window);
        void set_camera(std::shared_ptr<sub_system::ICameraController> camera);
        void set_cursor_config(const animation::CursorPoseConfig& config);
        void set_bone_names(const animation::BoneNameDictionary& dictionary);
        bool begin_model_drag(types::ClientMousePosition cursor);
        void end_model_drag();
        void post_update(void) override;
        void render(void) const override;

      private:
        [[nodiscard]] ModelBones& get_or_build(const types::HandleId entity,
            const component::ModelComponent& model,
            component::AnimationComponent& animation,
            foundation::Option<component::IKComponent&> ik,
            foundation::Option<component::PhysicsComponent&> physics,
            foundation::Option<component::PhysicsBodiesComponent&> physics_bodies) noexcept;

        void solve_ik(
            ModelBones& bones, foundation::Option<component::IKComponent&> opt_ik) const noexcept;
        void evaluate_bone_phase(ModelBones& bones,
            foundation::Option<component::IKComponent&> ik,
            bool after_physics) const noexcept;
        void execute_command(const types::SkinningCommand command,
            ModelBones& bones,
            foundation::Option<component::IKComponent&> ik) const noexcept;

        void write_skinning_matrices(const component::AnimationComponent& animation,
            const component::ModelComponent& model,
            component::SkinningComponent& skinning) const noexcept;
    };
} // namespace enishi::core
