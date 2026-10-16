#include "sdl3_window.h"
#include <foundation/log/logger.h>

namespace enishi::platform_impl {
    void SDL3Window::set_model_drag_handler(
        std::function<bool(types::ClientMousePosition)> begin, std::function<void()> end) {
        this->begin_model_drag = std::move(begin);
        this->end_model_drag = std::move(end);
    }

    void SDL3Window::refresh_mouse_position(void) {
        float mouse_x = 0.0f;
        float mouse_y = 0.0f;
        // Motion events stop outside the window; tracking must still notice the cursor leaving.
        SDL_GetGlobalMouseState(&mouse_x, &mouse_y);
        this->input.on_mouse_move(
            {static_cast<std::int32_t>(mouse_x), static_cast<std::int32_t>(mouse_y)});
    }

    void SDL3Window::cancel_model_drag(void) {
        if (this->model_drag_claimed && this->end_model_drag != nullptr) {
            this->end_model_drag();
        }
        this->model_drag_claimed = false;
        this->model_press.reset();
        this->model_dragging = false;
        if (!SDL_CaptureMouse(false)) {
            foundation::Logger::warning(SDL_GetError());
        }
    }

    void SDL3Window::update_model_drag(void) {
        float mouse_x = 0.0f;
        float mouse_y = 0.0f;
        SDL_GetGlobalMouseState(&mouse_x, &mouse_y);
        const auto dx = mouse_x - static_cast<float>(this->model_press->x);
        const auto dy = mouse_y - static_cast<float>(this->model_press->y);
        constexpr float DRAG_DISTANCE = 5.0f;
        // Distance squared = dx^2 + dy^2; window = initial window + cursor displacement.
        if (!this->model_dragging && dx * dx + dy * dy < DRAG_DISTANCE * DRAG_DISTANCE) {
            return;
        }
        if (!this->model_dragging && this->begin_model_drag != nullptr) {
            this->model_drag_claimed =
                this->begin_model_drag(types::to_client(*this->model_press, this->drag_origin));
        }
        this->model_dragging = true;
        if (this->model_drag_claimed) {
            return;
        }
        const auto result = this->set_position({this->drag_origin.x + static_cast<std::int32_t>(dx),
            this->drag_origin.y + static_cast<std::int32_t>(dy)});
        if (result.is_err()) {
            foundation::Logger::warning(result.unwrap_err().get_message());
        }
    }

    bool SDL3Window::handle_model_pointer(const SDL_Event& event, bool capture_mouse) {
        if (event.type == SDL_EVENT_WINDOW_FOCUS_LOST && this->model_press.has_value()) {
            this->cancel_model_drag();
            return false;
        }
        if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN && event.button.button == SDL_BUTTON_LEFT &&
            !capture_mouse && this->interactive_hit_test != nullptr) {
            const types::ClientMousePosition cursor{static_cast<std::int32_t>(event.button.x),
                static_cast<std::int32_t>(event.button.y)};
            if (!this->interactive_hit_test(cursor)) {
                return false;
            }
            this->model_press = types::to_screen(cursor, this->position);
            this->drag_origin = this->position;
            this->model_dragging = false;
            if (!SDL_CaptureMouse(true)) {
                foundation::Logger::warning(SDL_GetError());
            }
            return true;
        }
        if (!this->model_press.has_value()) {
            return false;
        }
        if (event.type == SDL_EVENT_MOUSE_MOTION) {
            this->update_model_drag();
            return true;
        }
        if (event.type == SDL_EVENT_MOUSE_BUTTON_UP && event.button.button == SDL_BUTTON_LEFT) {
            this->update_model_drag();
            if (!this->model_dragging && !capture_mouse) {
                // Publish a click only on release, after ruling out a window drag.
                this->input.on_mouse_move(*this->model_press);
                this->input.on_mouse_button_down(types::MouseButton::Left);
                this->input.on_mouse_button_up(types::MouseButton::Left);
            }
            this->cancel_model_drag();
            return true;
        }
        return false;
    }
} // namespace enishi::platform_impl
