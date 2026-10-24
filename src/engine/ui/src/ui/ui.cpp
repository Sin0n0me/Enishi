#include "ui.h"
#include <algorithm>
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

    void UI::draw_lines(std::span<const debug::OverlayLine> lines) {
        ImGui::SetCurrentContext(this->context.get());
        const auto viewport = ImGui::GetMainViewport();
        constexpr ImU32 OBB_COLOR = IM_COL32(64, 255, 96, 255);
        constexpr float LINE_THICKNESS = 1.5f;
        auto draw_list = ImGui::GetBackgroundDrawList();
        for (const auto& line : lines) {
            // UI position = viewport origin + normalized coordinate * logical viewport size.
            const ImVec2 start{viewport->Pos.x + line.start.x * viewport->Size.x,
                viewport->Pos.y + line.start.y * viewport->Size.y};
            const ImVec2 end{viewport->Pos.x + line.end.x * viewport->Size.x,
                viewport->Pos.y + line.end.y * viewport->Size.y};
            draw_list->AddLine(start, end, OBB_COLOR, LINE_THICKNESS);
        }
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

    bool UI::input_text_multiline(const char* label, std::string& value, bool read_only) {
        ImGui::SetCurrentContext(this->context.get());
        ImGui::TextUnformatted(label);
        const auto flags =
            read_only ? ImGuiInputTextFlags_ReadOnly : ImGuiInputTextFlags_AllowTabInput;
        constexpr float EDITOR_LINES = 8.0f;
        constexpr float ERROR_LINES = 5.0f;
        // Height = requested text lines * line height, plus the frame's top and bottom padding.
        const float lines = read_only ? ERROR_LINES : EDITOR_LINES;
        const float height =
            lines * ImGui::GetTextLineHeight() + 2.0f * ImGui::GetStyle().FramePadding.y;
        const std::string hidden_label = std::string("##") + label;
        return ImGui::InputTextMultiline(
            hidden_label.c_str(), &value, ImVec2{ImGui::GetContentRegionAvail().x, height}, flags);
    }

    bool UI::begin_context_popup(const char* id, float width, float height) {
        ImGui::SetCurrentContext(this->context.get());
        if (!ImGui::IsPopupOpen(id)) {
            const auto mouse = ImGui::GetMousePos();
            this->popup_anchor_x = mouse.x;
            this->popup_anchor_y = mouse.y;
        }
        if (ImGui::IsMouseReleased(ImGuiMouseButton_Right) &&
            !ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow) &&
            !ImGui::IsPopupOpen(nullptr, ImGuiPopupFlags_AnyPopupId)) {
            ImGui::OpenPopup(id);
        }
        const auto viewport = ImGui::GetMainViewport();
        constexpr float MINIMUM_EXTENT = 1.0f;
        if (viewport->WorkSize.x < MINIMUM_EXTENT || viewport->WorkSize.y < MINIMUM_EXTENT) {
            return false;
        }
        const ImVec2 size{
            std::clamp(width, MINIMUM_EXTENT, (std::max)(MINIMUM_EXTENT, viewport->WorkSize.x)),
            std::clamp(height, MINIMUM_EXTENT, (std::max)(MINIMUM_EXTENT, viewport->WorkSize.y))};
        // Clamp each page before Begin draws it: position = clamp(anchor, work_min, work_max -
        // size).
        const ImVec2 position{std::clamp(this->popup_anchor_x,
                                  viewport->WorkPos.x,
                                  viewport->WorkPos.x + viewport->WorkSize.x - size.x),
            std::clamp(this->popup_anchor_y,
                viewport->WorkPos.y,
                viewport->WorkPos.y + viewport->WorkSize.y - size.y)};
        ImGui::SetNextWindowPos(position);
        ImGui::SetNextWindowSize(size);
        ImGui::SetNextWindowSizeConstraints(ImVec2{MINIMUM_EXTENT, MINIMUM_EXTENT}, size);
        return ImGui::BeginPopup(id, ImGuiWindowFlags_HorizontalScrollbar);
    }

    void UI::end_popup(void) {
        ImGui::SetCurrentContext(this->context.get());
        ImGui::EndPopup();
    }

    bool UI::is_window_appearing(void) const {
        ImGui::SetCurrentContext(this->context.get());
        return ImGui::IsWindowAppearing();
    }

    void UI::same_line(void) {
        ImGui::SetCurrentContext(this->context.get());
        ImGui::SameLine();
    }

    void UI::begin_disabled(bool disabled) {
        ImGui::SetCurrentContext(this->context.get());
        ImGui::BeginDisabled(disabled);
    }

    void UI::end_disabled(void) {
        ImGui::SetCurrentContext(this->context.get());
        ImGui::EndDisabled();
    }

    void UI::push_id(const char* id) {
        ImGui::SetCurrentContext(this->context.get());
        ImGui::PushID(id);
    }

    void UI::pop_id(void) {
        ImGui::SetCurrentContext(this->context.get());
        ImGui::PopID();
    }

    void UI::tooltip(std::string_view value) {
        ImGui::SetCurrentContext(this->context.get());
        if (ImGui::IsItemHovered() && ImGui::BeginTooltip()) {
            this->text(value);
            ImGui::EndTooltip();
        }
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
