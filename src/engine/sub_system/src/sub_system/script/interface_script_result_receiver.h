#pragma once
#include "errors/script_errors.h"
#include "script_value.h"
#include <foundation/result/result.h>

namespace enishi::sub_system {
    template <typename T> using ScriptResult = foundation::Result<T, ScriptError>;

    class IScriptResultReceiver {
      public:
        virtual ~IScriptResultReceiver() noexcept = default;
        // Called on the caller's thread after a successful execution, in registration order.
        // Receivers can copy the value; failure stops dispatch without rolling back earlier
        // receivers.
        [[nodiscard]] virtual ScriptResult<void> on_script_result(const ScriptValue& value) = 0;
    };
} // namespace enishi::sub_system
