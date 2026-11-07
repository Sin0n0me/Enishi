#pragma once
#include <exception>
#include <source_location>
#include <utility>

namespace enishi::tests {
    void check(bool condition, const char* message,
        std::source_location location = std::source_location::current());
    [[noreturn]] void report_termination() noexcept;
    void begin_test(const char* name);
    void end_test(const char* name);

    template <typename Function> void run_test(const char* name, Function&& function) {
        begin_test(name);
        try {
            std::forward<Function>(function)();
        } catch (const std::exception& exception) {
            check(false, exception.what());
        } catch (...) {
            check(false, "unknown exception");
        }
        end_test(name);
    }
} // namespace enishi::tests
