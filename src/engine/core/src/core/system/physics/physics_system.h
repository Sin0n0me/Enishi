#pragma once
#include "../interface_system.h"
#include <ecs/registory.h>
#include <foundation/str/str.h>
#include <memory>
#include <platform/physics/interface_physics_engine.h>
#include <platform/physics/interface_physics_world.h>
#include <unordered_map>

namespace enishi::core {
    class PhysicsSystem : public ISystem {
      private:
        std::shared_ptr<ecs::Registory> registory;
        std::shared_ptr<platform::IPhysicsEngine> physics_engine;

        explicit PhysicsSystem(void) = delete;

      public:
        explicit PhysicsSystem(std::shared_ptr<ecs::Registory> registory,
            std::shared_ptr<platform::IPhysicsEngine> physics_engine);

      public:
        bool should_close(void) override;
        void pre_update(void) override;
        void post_update(void) override;
        void update(const types::DeltaTime& delta_time) override;
        void render(void) const override;

      private:
    };
} // namespace enishi::core
