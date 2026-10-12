#include <array>
#include <imgui.h>
#include <ui/ui.h>

using namespace enishi;
void require(bool condition, const char* message);
namespace {
    class Backend final : public sub_system::IUIBackend {
      public:
        ImVec2 size{800, 600};
        foundation::VoidResult<sub_system::UIError> init() override {
            auto& io = ImGui::GetIO();
            unsigned char* pixels = nullptr;
            int width = 0;
            int height = 0;
            io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
            return {};
        }
        void shutdown() noexcept override {
        }
        void begin_frame() override {
            ImGui::GetIO().DisplaySize = this->size;
            ImGui::GetIO().DeltaTime = 1.0f / 60.0f;
        }
        void render() override {
        }
    };
    void check_page(ui::UI& ui, float width, float height, bool close, bool expect_scroll = true) {
        ui.begin_frame();
        require(ui.begin_context_popup("bounds", width, height), "Popup did not open");
        const auto position = ImGui::GetWindowPos();
        const auto size = ImGui::GetWindowSize();
        const auto viewport = ImGui::GetMainViewport();
        if (expect_scroll) {
            require(ImGui::GetScrollMaxY() > 0.0f, "Tall content cannot be scrolled");
            require(ImGui::GetScrollMaxX() > 0.0f, "Wide content cannot be scrolled");
        }
        constexpr float TOLERANCE = 1.0f;
        require(position.x > viewport->WorkPos.x - TOLERANCE &&
                    position.y > viewport->WorkPos.y - TOLERANCE,
            "Popup above/left of viewport");
        require(position.x + size.x < viewport->WorkPos.x + viewport->WorkSize.x + TOLERANCE &&
                    position.y + size.y < viewport->WorkPos.y + viewport->WorkSize.y + TOLERANCE,
            "Popup clipped at right/bottom");
        for (int line = 0; line < 50; ++line) {
            ui.text(std::string(160, 'x'));
        }
        if (close) {
            ImGui::CloseCurrentPopup();
        }
        ui.end_popup();
        ui.end_frame();
    }
} // namespace
void popup_tests() {
    auto backend = std::make_shared<Backend>();
    ui::UI ui(backend);
    require(ui.init().is_ok(), "Headless UI initialization failed");
    constexpr std::array corners{ImVec2{1, 1}, ImVec2{799, 1}, ImVec2{1, 599}, ImVec2{799, 599}};
    for (const auto corner : corners) {
        backend->size = {800, 600};
        ui.begin_frame();
        ImGui::GetIO().MousePos = corner;
        require(!ui.begin_context_popup("bounds", 240, 180), "Closed popup unexpectedly open");
        ImGui::OpenPopup("bounds");
        if (ui.begin_context_popup("bounds", 240, 180)) {
            ui.end_popup();
        }
        ui.end_frame();
        check_page(ui, 240, 180, false, false);
        check_page(ui, 560, 360, false);
        check_page(ui, 640, 460, false);
        backend->size = {320, 240};
        check_page(ui, 640, 460, false);
        check_page(ui, 240, 180, true);
    }
}
