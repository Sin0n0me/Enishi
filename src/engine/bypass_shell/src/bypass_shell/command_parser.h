#pragma once
#include <cstddef>
#include <expected>
#include <string>
#include <string_view>
#include <vector>

namespace enishi::bypass_shell {
    enum class CommandSyntax { Cmd, Posix };
    enum class CommandParseErrorKind {
        EmptyInput,
        EmptyCommand,
        UnterminatedQuote,
        DanglingEscape,
        UnsupportedSyntax,
        InvalidCharacter,
        TooLong,
        InvalidSyntax
    };

    struct ParsedCommand {
        std::string name;
        std::vector<std::string> arguments;
    };

    struct CommandParseError {
        CommandParseErrorKind kind;
        std::size_t offset; // Byte offset in the original input.
        std::string message;
    };

    using CommandParseResult = std::expected<ParsedCommand, CommandParseError>;

    class CommandParser {
      public:
        // Input is UTF-8 text containing a complete command, not an arbitrary transport chunk.
        // Returns the first command; unquoted separators terminate parsing and the rest is ignored.
        // Variables and wildcards remain literal. These are lexical tokens, not the executed argv
        // (in particular, individual Windows programs have their own argument parsing rules).
        // Redirection, grouping and command substitution in the first command are unsupported.
        [[nodiscard]] static CommandParseResult parse(std::string_view input, CommandSyntax syntax);
    };
} // namespace enishi::bypass_shell
