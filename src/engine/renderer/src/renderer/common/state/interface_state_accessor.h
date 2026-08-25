#pragma once
#include <engine_types/handle/handle_type.h>
#include <engine_types/renderer/description/state_kind.h>
#include <foundation/option/option.h>
#include <memory>

namespace enishi::renderer {
    class IStateAccessor {
      public:
      public:
        virtual ~IStateAccessor(void) noexcept = default;

        [[nodiscard]] virtual foundation::Option<types::StateKind> get_state_kind(
            const types::HandleId& handle) const noexcept = 0;
    };
} // namespace enishi::renderer