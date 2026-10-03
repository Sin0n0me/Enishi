#pragma once
#include <engine_types/input/input_types.h>
#include <engine_types/input/key_code.h>
#include <engine_types/input/mouse_button.h>

namespace enishi::platform {
    class IInputEvent {
      public:
        virtual ~IInputEvent(void) noexcept = default;

        [[nodiscard]] virtual bool can_process_event(void) const noexcept = 0;

        virtual void on_key_down(const types::KeyCode code) = 0;

        virtual void on_key_up(const types::KeyCode code) = 0;

        virtual void on_mouse_move(const types::ScreenMousePosition position) = 0;

        virtual void on_mouse_button_down(const types::MouseButton button) = 0;

        virtual void on_mouse_button_up(const types::MouseButton button) = 0;

        virtual void begin_frame(void) = 0;

        virtual void end_frame(void) = 0;
    };
} // namespace enishi::platform