#include "model_click_system.h"

namespace enishi::core {
    ModelClickSystem::ModelClickSystem(
        std::shared_ptr<collider::Collider> collider, std::shared_ptr<platform::IWindow> window)
        : collider(std::move(collider))
        , window(std::move(window)) {
    }

    void ModelClickSystem::add_handler(std::shared_ptr<sub_system::IModelClickHandler> handler) {
        if (handler != nullptr) {
            this->handlers.push_back(std::move(handler));
        }
    }

    std::optional<types::ModelHit> ModelClickSystem::pick(types::ClientMousePosition cursor) const {
        const auto size = this->window->get_size();
        if (size.is_none() || cursor.x < 0 || cursor.y < 0 || !(cursor.x < size.unwrap().width) ||
            !(cursor.y < size.unwrap().height)) {
            return std::nullopt;
        }
        return this->collider->raycast(types::Ray::make_ray_from_screen(
            size.unwrap(), cursor, types::make_default_model_camera()));
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
        this->pending_click = types::ModelClick{cursor, this->pick(cursor)};
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

    bool ModelClickSystem::should_close(void) {
        return false;
    }
    void ModelClickSystem::post_update(void) {
    }
    void ModelClickSystem::render(void) const {
    }
} // namespace enishi::core
