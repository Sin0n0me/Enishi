#pragma once
#include <string>
#include <sub_system/script/interface_script_engine.h>
#include <sub_system/ui/interface_ui.h>
namespace enishi::ui {
    class SelectionScript final {
      private:
        std::string script_source = "fn main() -> i32 {\n    42\n}\n";
        std::string script_error;
        std::string script_status;

      public:
        void draw(sub_system::IUI& ui, sub_system::IScriptEngine& scripts);
    };
} // namespace enishi::ui
