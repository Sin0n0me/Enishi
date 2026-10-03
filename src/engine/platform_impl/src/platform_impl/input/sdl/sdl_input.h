#pragma once
#include <SDL3/SDL_keycode.h>
#include <SDL3/SDL_mouse.h>
#include <platform/input/interface_input.h>
#include <platform/input/interface_input_event.h>
#include <vector>

namespace enishi::platform_impl {
    class SDL3Input : public platform::IInput, platform::IInputEvent {
      private:
        std::vector<bool> key_state;
        std::vector<bool> pre_key_state;
        types::ScreenMousePosition mouse_position;
        types::ScreenMousePosition pre_mouse_position;
        types::MouseWheel mouse_wheel;
        types::MouseWheel pre_mouse_wheel;

      public:
        bool is_key_down(const types::KeyCode key) const noexcept override;
        bool is_key_pressed(const types::KeyCode key) const noexcept override;
        bool is_key_released(const types::KeyCode key) const noexcept override;
        types::ScreenMousePosition get_screen_mouse_position(void) const noexcept override;
        types::ClientMousePosition get_client_mouse_position(
            const types::WindowPosition& position) const noexcept override;
        types::MouseWheel get_mouse_wheel(void) const noexcept override;
        bool is_mouse_button_down(const types::MouseButton button) const noexcept override;
        bool is_mouse_button_pressed(const types::MouseButton button) const noexcept override;
        bool is_mouse_button_released(const types::MouseButton button) const noexcept override;

        bool can_process_event(void) const noexcept override;
        void on_key_down(const types::KeyCode code) override;
        void on_key_up(const types::KeyCode code) override;
        void on_mouse_move(const types::ScreenMousePosition position) override;
        void on_mouse_button_down(const types::MouseButton button) override;
        void on_mouse_button_up(const types::MouseButton button) override;
        void begin_frame(void) override;
        void end_frame(void) override;

        [[nodiscard]] static types::KeyCode convert_key_code(const SDL_Keycode& code);
        [[nodiscard]] static types::MouseButton convert_mouse_button(const std::uint8_t& code);
    };
} // namespace enishi::platform_impl