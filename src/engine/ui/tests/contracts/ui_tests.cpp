#include "ui_tests.h"
#include <test_support/test_runner.h>
#include <memory>
#include <ui/ui.h>

namespace enishi::tests {
    foundation::VoidResult<sub_system::UIError> RecordingUIBackend::init() {
        ++this->initializations;
        this->context = ImGui::GetCurrentContext();
        check(this->context != nullptr, "backend initialization has a current UI context");
        if (this->fail_init) {
            return foundation::Error(
                sub_system::UIError::InitializationFailed, "backend test failure");
        }
        auto& io = ImGui::GetIO();
        constexpr float WIDTH = 640.0f;
        constexpr float HEIGHT = 480.0f;
        io.DisplaySize = ImVec2(WIDTH, HEIGHT);
        unsigned char* pixels = nullptr;
        int width = 0;
        int height = 0;
        io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
        check(pixels != nullptr && width > 0 && height > 0, "headless font atlas is available");
        return {};
    }

    void RecordingUIBackend::shutdown() noexcept {
        check(ImGui::GetCurrentContext() == this->context, "shutdown runs with owning context");
        ++this->shutdowns;
    }

    void RecordingUIBackend::begin_frame() {
        check(ImGui::GetCurrentContext() == this->context, "frame begins with owning context");
        ++this->frames;
    }

    void RecordingUIBackend::render() {
        check(ImGui::GetCurrentContext() == this->context, "render uses owning context");
        check(ImGui::GetDrawData() != nullptr && ImGui::GetDrawData()->Valid,
            "backend receives completed draw data");
        ++this->renders;
    }

    void ui_lifecycle_tests() {
        ui::UI missing(nullptr);
        check(missing.init().is_err(), "missing backend is reported without dereferencing it");
        check(!missing.wants_capture_mouse() && !missing.wants_capture_keyboard(),
            "uninitialized UI does not capture input");
        missing.render();
        const auto backend = std::make_shared<RecordingUIBackend>();
        {
            ui::UI view(backend);
            backend->fail_init = true;
            const auto failure = view.init();
            check(failure.is_err() && failure.unwrap_err().get_message() == "backend test failure",
                "backend initialization error retains its diagnostic");
            check(backend->shutdowns == 1, "failed initialization releases backend resources");
            backend->fail_init = false;
            check(view.init().is_ok(), "failed initialization can be retried");
            check(view.init().is_err() && backend->initializations == 2,
                "duplicate initialization cannot initialize backend twice");
        }
        check(
            backend->shutdowns == 2, "successful initialization has exactly one matching shutdown");
    }

    void ui_frame_tests() {
        constexpr int FRAME_COUNT = 3;
        const auto first_backend = std::make_shared<RecordingUIBackend>();
        const auto second_backend = std::make_shared<RecordingUIBackend>();
        {
            ui::UI first(first_backend);
            ui::UI second(second_backend);
            check(first.init().is_ok() && second.init().is_ok(),
                "independent UI contexts initialize");
            first.render();
            check(first_backend->renders == 0, "render before a completed frame is ignored");
            for (int frame = 0; frame < FRAME_COUNT; ++frame) {
                for (auto* view : {&first, &second}) {
                    view->begin_frame();
                    if (view->begin_window("contract")) {
                        view->text("UTF-8: \xE6\x97\xA5\xE6\x9C\xAC\xE8\xAA\x9E");
                    }
                    view->end_window();
                    view->end_frame();
                }
                first.render();
                second.render();
                first.render();
            }
            check(first_backend->frames == FRAME_COUNT && first_backend->renders == FRAME_COUNT &&
                      second_backend->frames == FRAME_COUNT &&
                      second_backend->renders == FRAME_COUNT,
                "contexts can interleave; completed frames are rendered exactly once");
        }
        check(first_backend->shutdowns == 1 && second_backend->shutdowns == 1,
            "each context shuts down its own backend");
    }
} // namespace enishi::tests

int main() {
    enishi::tests::run_test("ui_lifecycle_tests", enishi::tests::ui_lifecycle_tests);
    enishi::tests::run_test("ui_frame_tests", enishi::tests::ui_frame_tests);
    return EXIT_SUCCESS;
}
