#pragma once
#include <cstdint>

namespace enishi::types {
    // このエンジンで扱うキーコード
    // 変換はWindowの実装先で行う
    enum class KeyCode : std::uint16_t {
        // Unknown
        KeyUnknown = 0,

        // Alphabet
        KeyA,
        KeyB,
        KeyC,
        KeyD,
        KeyE,
        KeyF,
        KeyG,
        KeyH,
        KeyI,
        KeyJ,
        KeyK,
        KeyL,
        KeyM,
        KeyN,
        KeyO,
        KeyP,
        KeyQ,
        KeyR,
        KeyS,
        KeyT,
        KeyU,
        KeyV,
        KeyW,
        KeyX,
        KeyY,
        KeyZ,

        // Number row
        Key0,
        Key1,
        Key2,
        Key3,
        Key4,
        Key5,
        Key6,
        Key7,
        Key8,
        Key9,

        // Function keys
        KeyF1,
        KeyF2,
        KeyF3,
        KeyF4,
        KeyF5,
        KeyF6,
        KeyF7,
        KeyF8,
        KeyF9,
        KeyF10,
        KeyF11,
        KeyF12,
        KeyF13,
        KeyF14,
        KeyF15,
        KeyF16,
        KeyF17,
        KeyF18,
        KeyF19,
        KeyF20,
        KeyF21,
        KeyF22,
        KeyF23,
        KeyF24,

        // Modifiers
        KeyShift,
        KeyLeftShift,
        KeyRightShift,

        KeyControl,
        KeyLeftControl,
        KeyRightControl,

        KeyAlt,
        KeyLeftAlt,
        KeyRightAlt,

        KeySuper, // Winキーなど
        KeyLeftSuper,
        KeyRightSuper,

        // Navigation
        KeyUp,
        KeyDown,
        KeyLeft,
        KeyRight,

        KeyHome,
        KeyEnd,
        KeyPageUp,
        KeyPageDown,
        KeyInsert,
        KeyDelete,

        // Editing
        KeyBackspace,
        KeyTab,
        KeyEnter,
        KeyEscape,
        KeySpace,

        // Lock keys
        KeyCapsLock,
        KeyNumLock,
        KeyScrollLock,

        // Print/System
        KeyPrintScreen,
        KeyPause,

        // Numpad
        KeyNumpad0,
        KeyNumpad1,
        KeyNumpad2,
        KeyNumpad3,
        KeyNumpad4,
        KeyNumpad5,
        KeyNumpad6,
        KeyNumpad7,
        KeyNumpad8,
        KeyNumpad9,

        KeyNumpadAdd,
        KeyNumpadSubtract,
        KeyNumpadMultiply,
        KeyNumpadDivide,
        KeyNumpadDecimal,
        KeyNumpadEnter,

        // Symbols
        KeyMinus,
        KeyEqual,
        KeyLeftBracket,
        KeyRightBracket,
        KeyBackslash,
        KeySemicolon,
        KeyApostrophe,
        KeyGrave,
        KeyComma,
        KeyPeriod,
        KeySlash,

        // Menu/Application
        KeyMenu,
    };
} // namespace enishi::types