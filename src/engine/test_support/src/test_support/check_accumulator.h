#pragma once
#include <cstddef>
#include <source_location>

namespace enishi::tests {
    class CheckAccumulator final {
      private:
        std::size_t failures = 0;

      public:
        void check(bool condition, const char* expression,
            std::source_location location = std::source_location::current());
        [[nodiscard]] std::size_t failure_count() const;
    };
} // namespace enishi::tests

#define ENISHI_EXPECT(checks, expression) (checks).check((expression), #expression)
