#pragma once
#include <debug/overlay/interface_debug_overlay.h>
#include <foundation/result/result.h>
#include <memory>
#include <sub_system/ui/interface_ui.h>
#include <sub_system/ui/interface_ui_backend.h>

struct ImGuiContext;

namespace enishi::ui {
    struct ImGuiContextDeleter {
        void operator()(ImGuiContext* context) const noexcept;
    };

    class UI final : public sub_system::IUI, public debug::IDebugOverlay {
      private:
        std::shared_ptr<sub_system::IUIBackend> backend;
        std::unique_ptr<ImGuiContext, ImGuiContextDeleter> context;
        bool initialized = false;
        bool frame_ready = false;
        float popup_anchor_x = 0.0f;
        float popup_anchor_y = 0.0f;

      public:
        explicit UI(std::shared_ptr<sub_system::IUIBackend> backend);
        ~UI(void) noexcept override;
        [[nodiscard]] foundation::VoidResult<sub_system::UIError> init(void);
        void begin_frame(void) override;
        void end_frame(void) override;
        void render(void) override;
        void draw_lines(std::span<const debug::OverlayLine> lines) override;
        void draw_window_border(void) override;
        bool begin_window(const char* title) override;
        void end_window(void) override;
        void text(std::string_view value) override;
        bool button(const char* label) override;
        bool input_text(const char* label, std::string& value) override;
        bool input_text_multiline(const char* label, std::string& value, bool read_only) override;
        bool begin_context_popup(const char* id, float width, float height) override;
        void end_popup(void) override;
        bool is_window_appearing(void) const override;
        void same_line(void) override;
        void begin_disabled(bool disabled) override;
        void end_disabled(void) override;
        void push_id(const char* id) override;
        void pop_id(void) override;
        void tooltip(std::string_view value) override;
        bool wants_capture_mouse(void) const override;
        bool wants_capture_keyboard(void) const override;

      private:
        void shutdown(void) noexcept;
    };
} // namespace enishi::ui
