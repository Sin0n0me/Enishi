#pragma once
#include "interface_script_result_receiver.h"
#include <memory>
#include <string_view>

namespace enishi::sub_system {
    class IScriptEngine {
      public:
        virtual ~IScriptEngine() noexcept = default;
        [[nodiscard]] virtual ScriptResult<ScriptValue> execute(std::string_view source) = 0;
        // Registration does not extend a receiver's lifetime. Duplicate registrations are ignored.
        virtual void add_result_receiver(std::shared_ptr<IScriptResultReceiver> receiver) = 0;
    };
} // namespace enishi::sub_system
