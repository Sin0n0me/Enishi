#include <core/system/ui/selection_ui.h>
#include <script/script_engine.h>
#include <utility>

using namespace enishi;
void require(bool condition, const char* message);
namespace {
    class Models final : public sub_system::IModelSelection {
      public:
        std::vector<sub_system::ModelEntry> get_models() const override {
            return {};
        }
        foundation::VoidResult<sub_system::ModelSelectionError> show_model(
            const std::string&) override {
            return {};
        }
        void hide_model(const std::string&) override {
        }
    };
    class UI final : public sub_system::IUI {
      public:
        std::string clicked;
        std::string source = "fn main() -> i32 { 42 }";
        std::string error;
        bool saw_editor = false;
        bool saw_script_button = false;
        void begin_frame() override {
        }
        void end_frame() override {
        }
        void render() override {
        }
        bool begin_window(const char*) override {
            return true;
        }
        void end_window() override {
        }
        void text(std::string_view) override {
        }
        bool button(const char* label) override {
            if (std::string_view(label) == "スクリプト") {
                this->saw_script_button = true;
            }
            if (this->clicked == label) {
                this->clicked.clear();
                return true;
            }
            return false;
        }
        bool input_text(const char*, std::string&) override {
            return false;
        }
        bool input_text_multiline(const char*, std::string& value, bool read_only) override {
            if (read_only) {
                this->error = value;
            } else {
                this->saw_editor = true;
                value = this->source;
            }
            return false;
        }
        bool begin_context_popup(const char*, float, float) override {
            return true;
        }
        void end_popup() override {
        }
        bool is_window_appearing() const override {
            return false;
        }
        void same_line() override {
        }
        void begin_disabled(bool) override {
        }
        void end_disabled() override {
        }
        void push_id(const char*) override {
        }
        void pop_id() override {
        }
        void tooltip(std::string_view) override {
        }
        bool wants_capture_mouse() const override {
            return false;
        }
        bool wants_capture_keyboard() const override {
            return false;
        }
    };
    class Receiver final : public sub_system::IScriptResultReceiver {
      public:
        int calls = 0;
        sub_system::ScriptResult<void> on_script_result(const sub_system::ScriptValue&) override {
            ++this->calls;
            return {};
        }
    };
} // namespace
void selection_tests() {
    auto engine = std::make_shared<script::ScriptEngine>();
    auto receiver = std::make_shared<Receiver>();
    engine->add_result_receiver(receiver);
    core::SelectionUI selection(
        std::make_shared<Models>(), std::make_shared<core::ModelClickSettings>(), engine);
    UI ui;
    ui.clicked = "スクリプト";
    selection.update(ui);
    require(ui.saw_script_button, "Script navigation button missing");
    selection.update(ui);
    require(ui.saw_editor, "Script editor was not displayed");
    ui.clicked = "実行";
    selection.update(ui);
    require(receiver->calls == 1 && ui.error.empty(), "UI execution did not notify receiver");
    ui.source = "fn main() { missing; }";
    ui.clicked = "実行";
    selection.update(ui);
    require(!ui.error.empty() && receiver->calls == 1, "UI compile error was not displayed");
    ui.source = "fn main() {}";
    ui.clicked = "実行";
    selection.update(ui);
    require(ui.error.empty() && receiver->calls == 2, "Old error remained after successful run");
    ui.clicked = "戻る";
    selection.update(ui);
    ui.saw_script_button = false;
    selection.update(ui);
    require(ui.saw_script_button, "Back button did not restore selection page");
}
