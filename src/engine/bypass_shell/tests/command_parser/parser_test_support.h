#pragma once
#include "../../../core/tests/test_runner.h"
#include <algorithm>
#include <bypass_shell/command_parser.h>

namespace enishi::tests {
    inline void print_parser_input(std::string_view input) {
        constexpr std::size_t DIAGNOSTIC_BYTES = 128;
        std::fprintf(stderr, "Input (%zu bytes), hex:", input.size());
        for (const unsigned char value : input.substr(0, DIAGNOSTIC_BYTES)) {
            std::fprintf(stderr, " %02x", static_cast<unsigned>(value));
        }
        std::fprintf(stderr, "\n");
    }
    inline void expect_command(std::string_view input,
        bypass_shell::CommandSyntax syntax,
        std::string_view name,
        const std::vector<std::string>& arguments,
        const std::source_location location = std::source_location::current()) {
        const auto result = bypass_shell::CommandParser::parse(input, syntax);
        if (!result.has_value()) {
            print_parser_input(input);
            std::fprintf(stderr,
                "Error at byte %zu: %s\n",
                result.error().offset,
                result.error().message.c_str());
        }
        check(result.has_value(), "Expected a parsed command", location);
        if (result->name != name || result->arguments != arguments) {
            print_parser_input(input);
            std::fprintf(stderr,
                "Expected %zu arguments, received %zu\n",
                arguments.size(),
                result->arguments.size());
        }
        check(result->name == name, "Command name mismatch", location);
        check(result->arguments == arguments, "Arguments mismatch", location);
    }
    inline void expect_error(std::string_view input,
        bypass_shell::CommandSyntax syntax,
        bypass_shell::CommandParseErrorKind kind,
        std::size_t offset,
        const std::source_location location = std::source_location::current()) {
        const auto result = bypass_shell::CommandParser::parse(input, syntax);
        if (result.has_value() || result.error().kind != kind || result.error().offset != offset) {
            print_parser_input(input);
            if (!result.has_value()) {
                std::fprintf(stderr,
                    "Error kind %d, byte %zu: %s\n",
                    static_cast<int>(result.error().kind),
                    result.error().offset,
                    result.error().message.c_str());
            }
        }
        check(!result.has_value(), "Expected a parse error", location);
        check(result.error().kind == kind, "Parse error kind mismatch", location);
        check(result.error().offset == offset, "Parse error offset mismatch", location);
        check(!result.error().message.empty(), "Error must contain a diagnostic", location);
    }
} // namespace enishi::tests
