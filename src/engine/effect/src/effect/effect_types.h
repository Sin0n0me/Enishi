#pragma once
#include <cstdint>
#include <foundation/result/result.h>

namespace enishi::effect {
    enum class EffectStatus { Running, Finished };
    enum class EffectError { InvalidArgument, Busy, IdExhausted, StartFailed, UpdateFailed };
    struct EffectId {
      public:
        using Value = std::uint64_t;
        static constexpr Value INVALID_VALUE = 0;
        Value value = INVALID_VALUE;
        [[nodiscard]] constexpr bool is_valid() const noexcept {
            return this->value != INVALID_VALUE;
        }
        constexpr bool operator==(const EffectId&) const noexcept = default;
    };
    struct EffectFailure {
        EffectId id;
        foundation::Error<EffectError> error;
    };
} // namespace enishi::effect
