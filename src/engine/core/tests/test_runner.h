#pragma once
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <source_location>
#include <utility>

namespace enishi::tests {
    inline const char* active_test = "startup";

    inline void check(bool condition,
        const char* message,
        const std::source_location location = std::source_location::current()) {
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

    [[noreturn]] inline void report_termination() noexcept {
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

    template <typename Function> void run_test(const char* name, Function&& function) {
        active_test = name;
        std::set_terminate(report_termination);
        std::fprintf(stderr, "[RUN ] %s\n", name);
        std::fflush(stderr);
        try {
            std::forward<Function>(function)();
        } catch (const std::exception& exception) {
            std::fprintf(stderr, "[FAIL] %s: %s\n", name, exception.what());
            std::exit(EXIT_FAILURE);
        } catch (...) {
            std::fprintf(stderr, "[FAIL] %s: unknown exception\n", name);
            std::exit(EXIT_FAILURE);
        }
        std::fprintf(stderr, "[PASS] %s\n", name);
        active_test = "between tests";
    }
} // namespace enishi::tests
