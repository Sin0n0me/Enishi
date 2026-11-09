#include "check_accumulator.h"
#include "test_runner.h"

namespace enishi::tests {
    void CheckAccumulator::check(bool condition, const char* expression,
        std::source_location location) {
        if (!condition) {
            ++this->failures;
            report_failure(expression, location);
        }
    }
    std::size_t CheckAccumulator::failure_count() const {
        return this->failures;
    }
} // namespace enishi::tests
