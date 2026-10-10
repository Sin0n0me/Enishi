#include "ui.h"
#include <imgui.h>
#include <misc/cpp/imgui_stdlib.h>
#include <utility>

namespace enishi::ui {
    void ImGuiContextDeleter::operator()(ImGuiContext* context) const noexcept {
        ImGui::DestroyContext(context);
    }

    UI::UI(std::shared_ptr<sub_system::IUIBackend> backend)
        : backend(std::move(backend)) {
    }

    UI::~UI(void) noexcept {
        this->shutdown();
    }

    foundation::VoidResult<sub_system::UIError> UI::init(void) {
        if (this->context != nullptr || this->backend == nullptr) {
            return foundation::Error(sub_system::UIError::InitializationFailed,
                "UI requires a backend and must not already be initialized");
        }
        IMGUI_CHECKVERSION();
        this->context.reset(ImGui::CreateContext());
        if (this->context == nullptr) {
            return foundation::Error(
                sub_system::UIError::InitializationFailed, "ImGui context creation failed");
        }
        ImGui::SetCurrentContext(this->context.get());
        ImGui::GetIO().IniFilename = nullptr;
        ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        ImGui::StyleColorsDark();
        auto result = this->backend->init();
        if (result.is_err()) {
            this->backend->shutdown();
            this->context.reset();
            return result;
        }
        this->initialized = true;
        return {};
    }

    void UI::shutdown(void) noexcept {
        if (this->context == nullptr) {
            return;
        }
        ImGui::SetCurrentContext(this->context.get());
        this->backend->shutdown();
        this->initialized = false;
        this->frame_ready = false;
        this->context.reset();
    }

    void UI::begin_frame(void) {
        ImGui::SetCurrentContext(this->context.get());
        this->frame_ready = false;
        this->backend->begin_frame();
        ImGui::NewFrame();
    }

    void UI::end_frame(void) {
        ImGui::SetCurrentContext(this->context.get());
        ImGui::Render();
        this->frame_ready = true;
    }

    void UI::render(void) {
        if (!this->frame_ready) {
            return;
        }
        ImGui::SetCurrentContext(this->context.get());
        this->backend->render();
        this->frame_ready = false;
    }

    bool UI::begin_window(const char* title) {
        ImGui::SetCurrentContext(this->context.get());
        return ImGui::Begin(title);
    }

    void UI::end_window(void) {
        ImGui::SetCurrentContext(this->context.get());
        ImGui::End();
    }

    void UI::text(std::string_view value) {
        ImGui::SetCurrentContext(this->context.get());
        if (value.empty()) {
            ImGui::TextUnformatted("");
            return;
        }
        ImGui::TextUnformatted(value.data(), value.data() + value.size());
    }

    bool UI::button(const char* label) {
        ImGui::SetCurrentContext(this->context.get());
        return ImGui::Button(label);
    }

    bool UI::input_text(const char* label, std::string& value) {
        ImGui::SetCurrentContext(this->context.get());
        return ImGui::InputText(label, &value);
    }

    bool UI::wants_capture_mouse(void) const {
        if (!this->initialized) {
            return false;
        }
        ImGui::SetCurrentContext(this->context.get());
        return ImGui::GetIO().WantCaptureMouse;
    }

    bool UI::wants_capture_keyboard(void) const {
        if (!this->initialized) {
            return false;
        }
        ImGui::SetCurrentContext(this->context.get());
        return ImGui::GetIO().WantCaptureKeyboard;
    }
} // namespace enishi::ui
