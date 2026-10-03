#pragma once
#include <foundation/result/result.h>
#include <memory>
#include <sub_system/ui/interface_ui.h>
#include <sub_system/ui/interface_ui_backend.h>

struct ImGuiContext;

namespace enishi::ui {
    struct ImGuiContextDeleter {
        void operator()(ImGuiContext* context) const noexcept;
    };

    class UI final : public sub_system::IUI {
      public:
        explicit UI(std::shared_ptr<sub_system::IUIBackend> backend);
        ~UI(void) noexcept override;
        [[nodiscard]] foundation::VoidResult<sub_system::UIError> init(void);
        void begin_frame(void) override;
        void end_frame(void) override;
        void render(void) override;
        bool begin_window(const char* title) override;
        void end_window(void) override;
        void text(std::string_view value) override;
        bool button(const char* label) override;
        bool input_text(const char* label, std::string& value) override;
        bool wants_capture_mouse(void) const override;
        bool wants_capture_keyboard(void) const override;

      private:
        std::shared_ptr<sub_system::IUIBackend> backend;
        std::unique_ptr<ImGuiContext, ImGuiContextDeleter> context;
        bool initialized = false;
        bool frame_ready = false;
        void shutdown(void) noexcept;
    };
} // namespace enishi::ui
