#pragma once
#include "../../errors/script_errors.h"
#include <foundation/result/result.h>

namespace enishi::platform {
    template <typename T>
    using ScriptResult = foundation::Result<T, foundation::Error<ScriptError>>;

    class IScriptEngne {
      private:
      public:
        [[nodiscard]] virtual ScriptResult<void> compile(void) = 0;

        [[nodiscard]] virtual ScriptResult<void> run_script(void) = 0;
    };
} // namespace enishi::platform