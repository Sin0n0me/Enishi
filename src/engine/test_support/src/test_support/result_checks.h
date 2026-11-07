#pragma once
#include "test_runner.h"
#include <string>
#include <type_traits>
#include <utility>

namespace enishi::tests {
    template <typename Result>
        requires requires(const Result& result) { result.is_ok(); result.unwrap_err().get_message(); }
    void check(const Result& result, const char* operation,
        std::source_location location = std::source_location::current()) {
        if (result.is_ok()) {
            return;
        }
        const auto& error = result.unwrap_err();
        std::string diagnostic = operation;
        if constexpr (requires { error.get_error(); }) {
            using Code = std::remove_cvref_t<decltype(error.get_error())>;
            if constexpr (std::is_enum_v<Code>) {
                diagnostic += " [error=" +
                    std::to_string(static_cast<std::underlying_type_t<Code>>(error.get_error())) + "]";
            }
        }
        const auto messages = error.get_message();
        diagnostic += "\n";
        diagnostic += messages.empty() ? "Result contains no error message" : messages;
        check(false, diagnostic.c_str(), location);
    }

    template <typename Result> void success(const Result& result,
        std::source_location location = std::source_location::current()) {
        check(result, "Expected successful Result", location);
    }

    template <typename Result>
        requires (!std::is_lvalue_reference_v<Result>)
    auto take(Result&& result,
        std::source_location location = std::source_location::current()) {
        check(result, "Expected Result value", location);
        return std::move(result).unwrap_mut();
    }
} // namespace enishi::tests
