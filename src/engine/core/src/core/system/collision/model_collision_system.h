#pragma once
#include <collider/collider.h>
#include <component/model_component.h>
#include <core/system/interface_system.h>
#include <ecs/registry.h>
#include <unordered_set>

namespace enishi::core {
    class ModelCollisionSystem : public ISystem {
      private:
        std::shared_ptr<ecs::Registry> registry;
        std::shared_ptr<collider::Collider> collider;
        std::unordered_set<types::CollisionModelId> registered;

      public:
        ModelCollisionSystem(
            std::shared_ptr<ecs::Registry> registry, std::shared_ptr<collider::Collider> collider);
        bool should_close(void) override;
        void pre_update(void) override;
        void update(const types::DeltaTime& delta_time) override;
        void post_update(void) override;
        void render(void) const override;
    };
} // namespace enishi::core
