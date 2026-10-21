#include "selection_ui.h"
#include <foundation/log/logger.h>

namespace enishi::ui {
    void SelectionUI::draw_script(sub_system::IUI& ui) {
        if (ui.button("戻る")) {
            this->page = Page::Actions;
            return;
        }
        ui.text("引数なしの main() を実行します");
        ui.input_text_multiline("スクリプト", this->script_source, false);
        if (ui.button("実行")) {
            this->script_error.clear();
            this->script_status.clear();
            auto result = this->scripts->execute(this->script_source);
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
