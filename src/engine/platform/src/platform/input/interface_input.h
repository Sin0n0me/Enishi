#pragma once
#include <engine_types/input/input_types.h>
#include <engine_types/input/key_code.h>
#include <engine_types/input/mouse_button.h>

namespace enishi::platform {
    class IInput {
      public:
        virtual ~IInput(void) noexcept = default;

        // キー
        [[nodiscard]] virtual bool is_key_down(const types::KeyCode key) const noexcept = 0;
        [[nodiscard]] virtual bool is_key_pressed(
            const types::KeyCode key) const noexcept = 0; // 今フレームだけ
        [[nodiscard]] virtual bool is_key_released(const types::KeyCode key) const noexcept = 0;

        // マウス
        [[nodiscard]] virtual types::ScreenMousePosition get_screen_mouse_position(
            void) const noexcept = 0;
        [[nodiscard]] virtual types::ClientMousePosition get_client_mouse_position(
            const types::WindowPosition& position) const noexcept = 0;
        [[nodiscard]] virtual types::MouseWheel get_mouse_wheel(void) const noexcept = 0;
        [[nodiscard]] virtual bool is_mouse_button_down(
            const types::MouseButton button) const noexcept = 0;
        [[nodiscard]] virtual bool is_mouse_button_pressed(
            const types::MouseButton button) const noexcept = 0; // 今フレームだけ
        [[nodiscard]] virtual bool is_mouse_button_released(
            const types::MouseButton button) const noexcept = 0;
    };
} // namespace enishi::platform