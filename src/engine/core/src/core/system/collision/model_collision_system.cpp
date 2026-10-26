#include "model_collision_system.h"
#include <component/animation_component.h>
#include <debug/log/logger.h>

namespace enishi::core {
    ModelCollisionSystem::ModelCollisionSystem(
        std::shared_ptr<ecs::Registry> registry, std::shared_ptr<collider::Collider> collider)
        : registry(std::move(registry))
        , collider(std::move(collider)) {
    }

    void ModelCollisionSystem::update_bounds(void) {
        std::unordered_set<types::CollisionModelId> visible;
        for (auto [entity, model, animation] :
            this->registry->view<component::ModelComponent, component::AnimationComponent>()) {
            if (!model.visible) {
                continue;
            }
            const types::CollisionModelId id = entity.handle_id;
            visible.insert(id);
            const bool initial = !this->registered.contains(id);
            if (initial) {
                if (!this->collider->register_model(id)) {
                    debug::Logger::warning("Cannot register model collision bounds");
                    continue;
                }
                this->registered.insert(id);
            }
            for (types::BoneIndex bone = 0; bone < model.collision_vertices.size(); ++bone) {
                const auto& vertices = model.collision_vertices[bone];
                if (vertices.empty() || !(bone < animation.global.size())) {
                    continue;
                }
                const bool updated =
                    initial ? this->collider->set_bone(id, bone, vertices, animation.global[bone])
                            : this->collider->update_bone(id, bone, animation.global[bone]);
                if (!updated) {
                    debug::Logger::warning("Cannot update model collision bounds");
                }
            }
        }
        for (auto it = this->registered.begin(); it != this->registered.end();) {
            if (!visible.contains(*it)) {
                this->collider->remove_model(*it);
                it = this->registered.erase(it);
            } else {
                ++it;
            }
        }
        this->collider->check_collisions();
    }

    void ModelCollisionSystem::pre_update(void) {
    }
    void ModelCollisionSystem::update(const types::DeltaTime& delta_time) {
    }
    void ModelCollisionSystem::post_update(void) {
        // Include visibility changes from the UI and the final pose submitted for this frame.
        this->update_bounds();
    }
    void ModelCollisionSystem::render(void) const {
    }
} // namespace enishi::core
