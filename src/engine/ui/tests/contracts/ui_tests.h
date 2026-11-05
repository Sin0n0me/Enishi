#pragma once
#include <imgui.h>
#include <sub_system/ui/interface_ui_backend.h>

namespace enishi::tests {
    class RecordingUIBackend final : public sub_system::IUIBackend {
      public:
        ImGuiContext* context = nullptr;
        bool fail_init = false;
        int initializations = 0;
        int shutdowns = 0;
        int frames = 0;
        int renders = 0;

      public:
        foundation::VoidResult<sub_system::UIError> init() override;
        void shutdown() noexcept override;
        void begin_frame() override;
        void render() override;
    };
    void ui_lifecycle_tests();
    void ui_frame_tests();
} // namespace enishi::tests
