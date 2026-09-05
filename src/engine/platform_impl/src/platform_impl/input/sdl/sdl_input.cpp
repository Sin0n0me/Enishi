#include "sdl_input.h"

namespace enishi::platform_impl {
    bool SDL3Input::is_key_down(const types::KeyCode key) const noexcept {
        const auto index = static_cast<std::uint64_t>(key);
        if (this->key_state.size() < index + 1) {
            return false;
        }
        return this->key_state[index];
    }

    bool SDL3Input::is_key_pressed(const types::KeyCode key) const noexcept {
        const auto index = static_cast<std::uint64_t>(key);
        if (this->key_state.size() < index + 1) {
            return false;
        }

        return this->key_state[index] && !this->pre_key_state[index];
    }

    bool SDL3Input::is_key_released(const types::KeyCode key) const noexcept {
        const auto index = static_cast<std::uint64_t>(key);
        if (this->key_state.size() < index + 1) {
            return false;
        }
        return !this->key_state[index];
    }

    types::ScreenMousePosition SDL3Input::get_screen_mouse_position(void) const noexcept {
        return this->mouse_position;
    }

    types::ClientMousePosition SDL3Input::get_client_mouse_position(
        const types::WindowPosition& position) const noexcept {
        return types::to_client(this->mouse_position, position);
    }

    types::MouseWheel SDL3Input::get_mouse_wheel(void) const noexcept {
        return types::MouseWheel();
    }
    bool SDL3Input::is_mouse_button_down(const types::MouseButton button) const noexcept {
        return false;
    }
    bool SDL3Input::is_mouse_button_pressed(const types::MouseButton button) const noexcept {
        return false;
    }
    bool SDL3Input::is_mouse_button_released(const types::MouseButton button) const noexcept {
        return false;
    }

    bool SDL3Input::can_process_event(void) const noexcept {
        return true;
    }

    void SDL3Input::on_key_down(const types::KeyCode code) {
        const auto index = static_cast<std::uint64_t>(code);
        if (this->key_state.size() < index + 1) {
            return;
        }

        this->pre_key_state[index] = this->key_state[index];
        this->key_state[index] = true;
    }

    void SDL3Input::on_key_up(const types::KeyCode code) {
        const auto index = static_cast<std::uint64_t>(code);
        if (this->key_state.size() < index + 1) {
            return;
        }

        this->pre_key_state[index] = this->key_state[index];
        this->key_state[index] = false;
    }

    void SDL3Input::on_mouse_move(const types::ScreenMousePosition position) {
    }

    void SDL3Input::on_mouse_button_down(const types::MouseButton button) {
    }

    void SDL3Input::on_mouse_button_up(const types::MouseButton button) {
    }

    void SDL3Input::begin_frame(void) {
    }

    void SDL3Input::end_frame(void) {
    }

    types::KeyCode SDL3Input::convert_key_code(const SDL_Keycode& code) {
        switch (code) {
            // Alphabet
            case SDLK_A:
                return types::KeyCode::KeyA;
            case SDLK_B:
                return types::KeyCode::KeyB;
            case SDLK_C:
                return types::KeyCode::KeyC;
            case SDLK_D:
                return types::KeyCode::KeyD;
            case SDLK_E:
                return types::KeyCode::KeyE;
            case SDLK_F:
                return types::KeyCode::KeyF;
            case SDLK_G:
                return types::KeyCode::KeyG;
            case SDLK_H:
                return types::KeyCode::KeyH;
            case SDLK_I:
                return types::KeyCode::KeyI;
            case SDLK_J:
                return types::KeyCode::KeyJ;
            case SDLK_K:
                return types::KeyCode::KeyK;
            case SDLK_L:
                return types::KeyCode::KeyL;
            case SDLK_M:
                return types::KeyCode::KeyM;
            case SDLK_N:
                return types::KeyCode::KeyN;
            case SDLK_O:
                return types::KeyCode::KeyO;
            case SDLK_P:
                return types::KeyCode::KeyP;
            case SDLK_Q:
                return types::KeyCode::KeyQ;
            case SDLK_R:
                return types::KeyCode::KeyR;
            case SDLK_S:
                return types::KeyCode::KeyS;
            case SDLK_T:
                return types::KeyCode::KeyT;
            case SDLK_U:
                return types::KeyCode::KeyU;
            case SDLK_V:
                return types::KeyCode::KeyV;
            case SDLK_W:
                return types::KeyCode::KeyW;
            case SDLK_X:
                return types::KeyCode::KeyX;
            case SDLK_Y:
                return types::KeyCode::KeyY;
            case SDLK_Z:
                return types::KeyCode::KeyZ;

            // Numbers
            case SDLK_0:
                return types::KeyCode::Key0;
            case SDLK_1:
                return types::KeyCode::Key1;
            case SDLK_2:
                return types::KeyCode::Key2;
            case SDLK_3:
                return types::KeyCode::Key3;
            case SDLK_4:
                return types::KeyCode::Key4;
            case SDLK_5:
                return types::KeyCode::Key5;
            case SDLK_6:
                return types::KeyCode::Key6;
            case SDLK_7:
                return types::KeyCode::Key7;
            case SDLK_8:
                return types::KeyCode::Key8;
            case SDLK_9:
                return types::KeyCode::Key9;

            // Function Keys
            case SDLK_F1:
                return types::KeyCode::KeyF1;
            case SDLK_F2:
                return types::KeyCode::KeyF2;
            case SDLK_F3:
                return types::KeyCode::KeyF3;
            case SDLK_F4:
                return types::KeyCode::KeyF4;
            case SDLK_F5:
                return types::KeyCode::KeyF5;
            case SDLK_F6:
                return types::KeyCode::KeyF6;
            case SDLK_F7:
                return types::KeyCode::KeyF7;
            case SDLK_F8:
                return types::KeyCode::KeyF8;
            case SDLK_F9:
                return types::KeyCode::KeyF9;
            case SDLK_F10:
                return types::KeyCode::KeyF10;
            case SDLK_F11:
                return types::KeyCode::KeyF11;
            case SDLK_F12:
                return types::KeyCode::KeyF12;
            case SDLK_F13:
                return types::KeyCode::KeyF13;
            case SDLK_F14:
                return types::KeyCode::KeyF14;
            case SDLK_F15:
                return types::KeyCode::KeyF15;
            case SDLK_F16:
                return types::KeyCode::KeyF16;
            case SDLK_F17:
                return types::KeyCode::KeyF17;
            case SDLK_F18:
                return types::KeyCode::KeyF18;
            case SDLK_F19:
                return types::KeyCode::KeyF19;
            case SDLK_F20:
                return types::KeyCode::KeyF20;
            case SDLK_F21:
                return types::KeyCode::KeyF21;
            case SDLK_F22:
                return types::KeyCode::KeyF22;
            case SDLK_F23:
                return types::KeyCode::KeyF23;
            case SDLK_F24:
                return types::KeyCode::KeyF24;

            // Modifiers
            case SDLK_LSHIFT:
                return types::KeyCode::KeyLeftShift;
            case SDLK_RSHIFT:
                return types::KeyCode::KeyRightShift;
            case SDLK_LCTRL:
                return types::KeyCode::KeyLeftControl;
            case SDLK_RCTRL:
                return types::KeyCode::KeyRightControl;
            case SDLK_LALT:
                return types::KeyCode::KeyLeftAlt;
            case SDLK_RALT:
                return types::KeyCode::KeyRightAlt;
            case SDLK_LGUI:
                return types::KeyCode::KeyLeftSuper;
            case SDLK_RGUI:
                return types::KeyCode::KeyRightSuper;

            // Navigation
            case SDLK_UP:
                return types::KeyCode::KeyUp;
            case SDLK_DOWN:
                return types::KeyCode::KeyDown;
            case SDLK_LEFT:
                return types::KeyCode::KeyLeft;
            case SDLK_RIGHT:
                return types::KeyCode::KeyRight;
            case SDLK_HOME:
                return types::KeyCode::KeyHome;
            case SDLK_END:
                return types::KeyCode::KeyEnd;
            case SDLK_PAGEUP:
                return types::KeyCode::KeyPageUp;
            case SDLK_PAGEDOWN:
                return types::KeyCode::KeyPageDown;
            case SDLK_INSERT:
                return types::KeyCode::KeyInsert;
            case SDLK_DELETE:
                return types::KeyCode::KeyDelete;

            // Editing
            case SDLK_BACKSPACE:
                return types::KeyCode::KeyBackspace;
            case SDLK_TAB:
                return types::KeyCode::KeyTab;
            case SDLK_RETURN:
                return types::KeyCode::KeyEnter;
            case SDLK_ESCAPE:
                return types::KeyCode::KeyEscape;
            case SDLK_SPACE:
                return types::KeyCode::KeySpace;

            // Locks
            case SDLK_CAPSLOCK:
                return types::KeyCode::KeyCapsLock;
            case SDLK_NUMLOCKCLEAR:
                return types::KeyCode::KeyNumLock;
            case SDLK_SCROLLLOCK:
                return types::KeyCode::KeyScrollLock;

            // System
            case SDLK_PRINTSCREEN:
                return types::KeyCode::KeyPrintScreen;
            case SDLK_PAUSE:
                return types::KeyCode::KeyPause;

            // Numpad
            case SDLK_KP_0:
                return types::KeyCode::KeyNumpad0;
            case SDLK_KP_1:
                return types::KeyCode::KeyNumpad1;
            case SDLK_KP_2:
                return types::KeyCode::KeyNumpad2;
            case SDLK_KP_3:
                return types::KeyCode::KeyNumpad3;
            case SDLK_KP_4:
                return types::KeyCode::KeyNumpad4;
            case SDLK_KP_5:
                return types::KeyCode::KeyNumpad5;
            case SDLK_KP_6:
                return types::KeyCode::KeyNumpad6;
            case SDLK_KP_7:
                return types::KeyCode::KeyNumpad7;
            case SDLK_KP_8:
                return types::KeyCode::KeyNumpad8;
            case SDLK_KP_9:
                return types::KeyCode::KeyNumpad9;
            case SDLK_KP_PLUS:
                return types::KeyCode::KeyNumpadAdd;
            case SDLK_KP_MINUS:
                return types::KeyCode::KeyNumpadSubtract;
            case SDLK_KP_MULTIPLY:
                return types::KeyCode::KeyNumpadMultiply;
            case SDLK_KP_DIVIDE:
                return types::KeyCode::KeyNumpadDivide;
            case SDLK_KP_PERIOD:
                return types::KeyCode::KeyNumpadDecimal;
            case SDLK_KP_ENTER:
                return types::KeyCode::KeyNumpadEnter;

            // Symbols
            case SDLK_MINUS:
                return types::KeyCode::KeyMinus;
            case SDLK_EQUALS:
                return types::KeyCode::KeyEqual;
            case SDLK_LEFTBRACKET:
                return types::KeyCode::KeyLeftBracket;
            case SDLK_RIGHTBRACKET:
                return types::KeyCode::KeyRightBracket;
            case SDLK_BACKSLASH:
                return types::KeyCode::KeyBackslash;
            case SDLK_SEMICOLON:
                return types::KeyCode::KeySemicolon;
            case SDLK_APOSTROPHE:
                return types::KeyCode::KeyApostrophe;
            case SDLK_GRAVE:
                return types::KeyCode::KeyGrave;
            case SDLK_COMMA:
                return types::KeyCode::KeyComma;
            case SDLK_PERIOD:
                return types::KeyCode::KeyPeriod;
            case SDLK_SLASH:
                return types::KeyCode::KeySlash;

            // Menu
            case SDLK_APPLICATION:
                return types::KeyCode::KeyMenu;

            default:
                return types::KeyCode::KeyUnknown;
        }
    }

    types::MouseButton SDL3Input::convert_mouse_button(const std::uint8_t& code) {
        switch (code) {
            case SDL_BUTTON_LEFT:
                return types::MouseButton::Left;
            case SDL_BUTTON_MIDDLE:
                return types::MouseButton::Middle;
            case SDL_BUTTON_RIGHT:
                return types::MouseButton::Right;
            case SDL_BUTTON_X1:
                return types::MouseButton::Button4;
            case SDL_BUTTON_X2:
                return types::MouseButton::Button5;
            default:
                break;
        }

        return types::MouseButton::None;
    }
} // namespace enishi::platform_impl