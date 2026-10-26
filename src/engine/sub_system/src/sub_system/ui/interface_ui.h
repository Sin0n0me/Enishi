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
        virtual bool input_text_multiline(
            const char* label, std::string& value, bool read_only) = 0;
        // End only popups for which begin_context_popup returned true.
        [[nodiscard]] virtual bool begin_context_popup(
            const char* id, float width, float height) = 0;
        virtual void end_popup(void) = 0;
        [[nodiscard]] virtual bool is_window_appearing(void) const = 0;
        virtual void same_line(void) = 0;
        virtual void begin_disabled(bool disabled) = 0;
        virtual void end_disabled(void) = 0;
        virtual void push_id(const char* id) = 0;
        virtual void pop_id(void) = 0;
        virtual void tooltip(std::string_view value) = 0;
        [[nodiscard]] virtual bool wants_capture_mouse(void) const = 0;
        [[nodiscard]] virtual bool wants_capture_keyboard(void) const = 0;
    };
} // namespace enishi::sub_system
