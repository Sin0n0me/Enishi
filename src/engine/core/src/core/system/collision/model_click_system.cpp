#include "model_click_system.h"
#include <component/physics_bodies_component.h>
#include <limits>
#include <unordered_set>

namespace enishi::core {
    ModelClickSystem::ModelClickSystem(std::shared_ptr<collider::Collider> collider,
        std::shared_ptr<platform::IWindow> window,
        std::shared_ptr<ecs::Registry> registry,
        std::shared_ptr<ModelClickSettings> settings,
        std::shared_ptr<sub_system::ICameraController> camera)
        : collider(std::move(collider))
        , window(std::move(window))
        , registry(std::move(registry))
        , settings(std::move(settings))
        , camera(std::move(camera)) {
    }

    void ModelClickSystem::add_handler(std::shared_ptr<sub_system::IModelClickHandler> handler) {
        if (handler != nullptr) {
            this->handlers.push_back(std::move(handler));
        }
    }

    std::optional<types::ModelHit> ModelClickSystem::pick(
        types::ClientMousePosition cursor, bool apply_click_mode) const {
        const auto size = this->window->get_size();
        if (this->camera == nullptr || size.is_none() || cursor.x < 0 || cursor.y < 0 ||
            !(cursor.x < size.unwrap().width) || !(cursor.y < size.unwrap().height)) {
            return std::nullopt;
        }
        const auto ray =
            types::Ray::make_ray_from_screen(size.unwrap(), cursor, this->camera->get_camera());
        const auto hit = this->collider->raycast(ray);
        if (!apply_click_mode || this->settings->mode != ModelClickMode::Push || !hit.has_value()) {
            return hit;
        }
        if (hit->model > (std::numeric_limits<types::HandleId::HandleType>::max)()) {
            return std::nullopt;
        }
        const types::HandleId entity(static_cast<types::HandleId::HandleType>(hit->model));
        const auto bodies = this->registry->get<component::PhysicsBodiesComponent>(entity);
        if (bodies.is_none() || !bodies.unwrap().enabled || !bodies.unwrap().simulation_enabled) {
            return std::nullopt;
        }
        std::unordered_set<types::BoneIndex> selected_bones;
        for (const auto& body : bodies.unwrap().rigid_bodies) {
            if (this->settings->accepts_push(body.kind)) {
                selected_bones.insert(body.relate_bone_index);
            }
        }
        // Only configured states participate, including zero-mass bodies for click diagnostics.
        // Keep the nearest model so the push cannot pass through to another character.
        return this->collider->raycast(
            ray, [&](types::CollisionModelId model, types::BoneIndex bone) {
                return model == hit->model && selected_bones.contains(bone);
            });
    }

    void ModelClickSystem::pre_update(void) {
        this->pending_click.reset();
        const auto input = this->window->get_input();
        const auto position = this->window->get_position();
        if (input.is_none() || position.is_none() ||
            !input.unwrap()->is_mouse_button_pressed(types::MouseButton::Left)) {
            return;
        }
        const auto cursor =
            input.unwrap()->get_mouse_press_position(types::MouseButton::Left, position.unwrap());
        // Pick the displayed pose before animation and physics advance to the next frame.
        this->pending_click = types::ModelClick{cursor, this->pick(cursor, true)};
    }

    void ModelClickSystem::update(const types::DeltaTime& delta_time) {
        if (!this->pending_click.has_value()) {
            return;
        }
        const auto event = *this->pending_click;
        this->pending_click.reset();
        // Snapshot listeners so registration from a callback does not invalidate iteration.
        const auto listeners = this->handlers;
        for (const auto& handler : listeners) {
            handler->on_model_click(event);
        }
    }

    void ModelClickSystem::post_update(void) {
    }
    void ModelClickSystem::render(void) const {
    }
} // namespace enishi::core
