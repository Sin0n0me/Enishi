#include "selection_script.h"

namespace enishi::ui {
    void SelectionScript::draw(sub_system::IUI& ui, sub_system::IScriptEngine& scripts) {
        ui.text("引数なしの main() を実行します");
        ui.input_text_multiline("スクリプト", this->script_source, false);
        if (ui.button("実行")) {
            this->script_error.clear();
            this->script_status.clear();
            auto result = scripts.execute(this->script_source);
            if (result.is_err()) {
                this->script_error = result.unwrap_err().get_message();
            } else {
                this->script_status = "実行しました";
            }
        }
        ui.text(this->script_status);
        ui.input_text_multiline("スクリプトエラー", this->script_error, true);
    }

} // namespace enishi::ui
