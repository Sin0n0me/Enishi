#include "test_runner.h"
#include <cstdio>
#include <cstdlib>

namespace enishi::tests {
    namespace {
        const char* active_test = "startup";
    }
    void check(bool condition,
        const char* message,
        const std::source_location location) {
        if (!condition) {
            std::fprintf(stderr,
                "[FAIL] %s: %s (%s:%u)\n",
                active_test,
                message,
                location.file_name(),
                location.line());
            std::fflush(stderr);
            std::exit(EXIT_FAILURE);
        }
    }

    [[noreturn]] void report_termination() noexcept {
        std::fprintf(stderr, "[FAIL] %s: std::terminate", active_test);
        const auto error = std::current_exception();
        if (error != nullptr) {
            try {
                std::rethrow_exception(error);
            } catch (const std::exception& exception) {
                std::fprintf(stderr, ": %s", exception.what());
            } catch (...) {
                std::fprintf(stderr, ": unknown exception");
            }
        } else {
            std::fprintf(stderr, " without an active exception");
        }
        std::fprintf(stderr, "\n");
        std::fflush(stderr);
        // Termination may originate in noexcept code; do not resume a damaged test process.
        std::_Exit(EXIT_FAILURE);
    }

    void require(bool condition, const char* message, std::source_location location) {
        check(condition, message, location);
    }
    void begin_test(const char* name) {
        active_test = name;
        std::set_terminate(report_termination);
        std::fprintf(stderr, "[RUN ] %s\n", name);
        std::fflush(stderr);
    }
    void end_test(const char* name) {
        std::fprintf(stderr, "[PASS] %s\n", name);
        std::fflush(stderr);
        active_test = "between tests";
    }
} // namespace enishi::tests
