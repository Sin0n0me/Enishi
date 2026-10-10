#include "physics_system.h"
#include "impulse_morph_application.h"
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
        auto* world = this->physics_engine->get_world();
        for (auto [entity, bodies] : this->registry->view<component::PhysicsBodiesComponent>()) {
            if (bodies.enabled == bodies.simulation_enabled) {
                continue;
            }
            bool succeeded = true;
            for (const auto& handle : bodies.handles) {
                const auto result = world->set_body_enabled(handle, bodies.enabled);
                if (result.is_err()) {
                    foundation::Logger::warning(result.unwrap_err().get_message());
                    succeeded = false;
                }
            }
            if (succeeded) {
                bodies.simulation_enabled = bodies.enabled;
            }
        }
        if (world->get_config_reader()->can_update()) {
            for (auto [entity, model, morph, bodies] :
                this->registry->view<component::ModelComponent,
                    component::MorphComponent,
                    component::PhysicsBodiesComponent>()) {
                if (!bodies.enabled) {
                    continue;
                }
                const auto result = apply_impulse_morphs(*world, model, morph, bodies);
                if (result.is_err()) {
                    foundation::Logger::warning(result.unwrap_err().get_message());
                }
            }
        }
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
