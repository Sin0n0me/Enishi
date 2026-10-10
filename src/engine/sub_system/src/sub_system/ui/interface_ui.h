#pragma once
#include <string>
#include <string_view>

namespace enishi::sub_system {
    class IUI {
      public:
        virtual ~IUI(void) noexcept = default;
        virtual void begin_frame(void) = 0;
        virtual void end_frame(void) = 0;
        // Draw into the current render target before it is presented.
        virtual void render(void) = 0;
        // Pair every begin_window with end_window, including collapsed windows.
        [[nodiscard]] virtual bool begin_window(const char* title) = 0;
        virtual void end_window(void) = 0;
        virtual void text(std::string_view value) = 0;
        [[nodiscard]] virtual bool button(const char* label) = 0;
        // The caller owns the UTF-8 value, which is updated when edited.
        virtual bool input_text(const char* label, std::string& value) = 0;
        [[nodiscard]] virtual bool wants_capture_mouse(void) const = 0;
        [[nodiscard]] virtual bool wants_capture_keyboard(void) const = 0;
    };
} // namespace enishi::sub_system
