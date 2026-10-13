#pragma once
#include <sub_system/script/interface_script_engine.h>
#include <vector>

namespace enishi::script {
    class ScriptEngine final : public sub_system::IScriptEngine {
      private:
        std::vector<std::weak_ptr<sub_system::IScriptResultReceiver>> receivers;

      public:
        [[nodiscard]] sub_system::ScriptResult<sub_system::ScriptValue> execute(
            std::string_view source) override;
        void add_result_receiver(
            std::shared_ptr<sub_system::IScriptResultReceiver> receiver) override;
    };
} // namespace enishi::script
