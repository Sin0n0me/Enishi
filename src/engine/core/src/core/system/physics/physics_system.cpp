#include "physics_system.h"
#include <component/animation_component.h>
#include <component/model_component.h>
#include <core/system/skinning/skinning_system.h>
#include <foundation/log/logger.h>

namespace enishi::core {
    PhysicsSystem::PhysicsSystem(std::shared_ptr<ecs::Registry> registry,
        std::shared_ptr<sub_system::IPhysicsEngine> physics_engine,
        std::shared_ptr<SkinningSystem> skinning_system)
        : registry(registry)
        , physics_engine(std::move(physics_engine))
        , skinning_system(std::move(skinning_system)) {
    }

    bool PhysicsSystem::should_close(void) {
        return false;
    }

    void PhysicsSystem::pre_update(void) {
    }

    void PhysicsSystem::update(const types::DeltaTime& delta_time) {
        this->physics_engine->update(delta_time);
        this->physics_engine->get_world()->apply_physics();
        if (this->skinning_system != nullptr) {
            this->skinning_system->update_after_physics();
        }
    }

    void PhysicsSystem::post_update(void) {
    }

    void PhysicsSystem::render(void) const {
    }
} // namespace enishi::core
