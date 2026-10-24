#pragma once
#include <debug/playback/playback_control.h>
#include "../interface_system.h"
#include <ecs/registry.h>
#include <foundation/str/str.h>
#include <memory>
#include <sub_system/physics/interface_physics_engine.h>
#include <sub_system/physics/interface_physics_world.h>
#include <unordered_map>

namespace enishi::core {
    class SkinningSystem;
    class PhysicsSystem : public ISystem {
      private:
        std::shared_ptr<ecs::Registry> registry;
        std::shared_ptr<debug::PlaybackControl> debug_playback;
        std::shared_ptr<sub_system::IPhysicsEngine> physics_engine;
        std::shared_ptr<SkinningSystem> skinning_system;

        explicit PhysicsSystem(void) = delete;

      public:
        explicit PhysicsSystem(std::shared_ptr<ecs::Registry> registry,
            std::shared_ptr<sub_system::IPhysicsEngine> physics_engine,
            std::shared_ptr<SkinningSystem> skinning_system = nullptr);

      public:
        void set_debug_playback(std::shared_ptr<debug::PlaybackControl> playback);
        void pre_update(void) override;
        void post_update(void) override;
        void update(const types::DeltaTime& delta_time) override;
        void render(void) const override;

      private:
    };
} // namespace enishi::core
