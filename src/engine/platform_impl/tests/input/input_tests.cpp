#include "input_tests.h"
#include "../../../core/tests/test_runner.h"
#include <limits>
#include <platform_impl/input/sdl/sdl_input.h>

namespace enishi::tests {
    void keyboard_contract_tests() {
        platform_impl::SDL3Input input;
        constexpr auto KEY = types::KeyCode::KeyA;
        check(!input.is_key_down(KEY) && !input.is_key_pressed(KEY) && !input.is_key_released(KEY),
            "keys start released without transition events");
        input.on_key_down(KEY);
        input.on_key_down(KEY);
        check(input.is_key_down(KEY) && input.is_key_pressed(KEY),
            "initial press is retained across repeats");
        input.begin_frame();
        input.on_key_down(KEY);
        check(input.is_key_down(KEY) && !input.is_key_pressed(KEY), "repeat is not a new press");
        input.on_key_up(KEY);
        input.on_key_up(KEY);
        check(!input.is_key_down(KEY) && input.is_key_released(KEY),
            "release edge survives repeated up events");
        input.begin_frame();
        input.on_key_down(KEY);
        input.on_key_up(KEY);
        check(input.is_key_pressed(KEY) && input.is_key_released(KEY) && !input.is_key_down(KEY),
            "a complete tap within one frame is not lost");
        input.reset_keys();
        check(!input.is_key_down(KEY) && !input.is_key_pressed(KEY) && !input.is_key_released(KEY),
            "focus reset clears both state and transitions");
        constexpr auto INVALID =
            static_cast<types::KeyCode>(std::numeric_limits<std::uint16_t>::max());
        input.on_key_down(INVALID);
        input.on_key_up(INVALID);
        check(!input.is_key_down(INVALID) && !input.is_key_pressed(INVALID) &&
                  !input.is_key_released(INVALID),
            "out-of-range key values are ignored safely");
        check(platform_impl::SDL3Input::convert_key_code(SDLK_A) == KEY &&
                  platform_impl::SDL3Input::convert_key_code(SDLK_APPLICATION) ==
                      types::KeyCode::KeyMenu &&
                  platform_impl::SDL3Input::convert_key_code(SDLK_UNKNOWN) ==
                      types::KeyCode::KeyUnknown,
            "SDL key translation preserves known keys and rejects unknown keys");
    }

    void pointer_contract_tests() {
        platform_impl::SDL3Input input;
        constexpr types::WindowPosition WINDOW{-1920, -400};
        constexpr types::ScreenMousePosition PRESS{-1800, -350};
        constexpr types::ScreenMousePosition MOVED{-1700, -300};
        constexpr types::ClientMousePosition LOCAL_PRESS{120, 50};
        constexpr types::ClientMousePosition LOCAL_MOVED{220, 100};
        input.on_mouse_move(PRESS);
        input.on_mouse_button_down(types::MouseButton::Left);
        input.on_mouse_move(MOVED);
        input.on_mouse_button_down(types::MouseButton::Left);
        const auto press = input.get_mouse_press_position(types::MouseButton::Left, WINDOW);
        const auto current = input.get_client_mouse_position(WINDOW);
        check(press.x == LOCAL_PRESS.x && press.y == LOCAL_PRESS.y && current.x == LOCAL_MOVED.x &&
                  current.y == LOCAL_MOVED.y,
            "negative monitor coordinates convert correctly; repeats preserve original press "
            "position");
        input.begin_frame();
        check(input.is_mouse_button_down(types::MouseButton::Left) &&
                  !input.is_mouse_button_pressed(types::MouseButton::Left),
            "held buttons survive frame transitions without extra press events");
        input.on_mouse_button_up(types::MouseButton::Left);
        check(!input.is_mouse_button_down(types::MouseButton::Left) &&
                  input.is_mouse_button_released(types::MouseButton::Left),
            "mouse release is observable");
        constexpr float FRACTION = 0.25f;
        constexpr float ACCUMULATED = 0.5f;
        input.on_mouse_wheel({FRACTION, -FRACTION});
        input.on_mouse_wheel({FRACTION, -FRACTION});
        input.on_mouse_wheel({std::numeric_limits<float>::quiet_NaN(), 0});
        input.on_mouse_wheel({0, std::numeric_limits<float>::infinity()});
        check(input.get_mouse_wheel().x == ACCUMULATED && input.get_mouse_wheel().y == -ACCUMULATED,
            "fractional touchpad events accumulate; nonfinite events do not corrupt the frame");
        input.begin_frame();
        check(input.get_mouse_wheel().x == 0 && input.get_mouse_wheel().y == 0 &&
                  !input.is_mouse_button_released(types::MouseButton::Left),
            "wheel and release edges reset at the next frame");
        constexpr auto INVALID =
            static_cast<types::MouseButton>(std::numeric_limits<std::uint8_t>::max());
        for (const auto button : {types::MouseButton::None, INVALID}) {
            input.on_mouse_button_down(button);
            input.on_mouse_button_up(button);
            check(!input.is_mouse_button_down(button) && !input.is_mouse_button_pressed(button) &&
                      !input.is_mouse_button_released(button),
                "invalid mouse buttons are ignored");
        }
        check(platform_impl::SDL3Input::convert_mouse_button(SDL_BUTTON_RIGHT) ==
                      types::MouseButton::Right &&
                  platform_impl::SDL3Input::convert_mouse_button(SDL_BUTTON_X1) ==
                      types::MouseButton::Button4,
            "SDL mouse numbering is translated rather than cast to engine numbering");
    }
} // namespace enishi::tests

int main() {
    enishi::tests::run_test("keyboard_contract_tests", enishi::tests::keyboard_contract_tests);
    enishi::tests::run_test("pointer_contract_tests", enishi::tests::pointer_contract_tests);
    return EXIT_SUCCESS;
}
