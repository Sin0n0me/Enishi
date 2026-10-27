#include "command_parser.h"
#include <utility>

namespace enishi::bypass_shell {
    namespace {
        constexpr std::size_t MAX_COMMAND_BYTES = 65536;
        constexpr unsigned char FIRST_PRINTABLE = 32;
        constexpr unsigned char DELETE_CHARACTER = 127;
        enum class Quote { None, Single, Double };

        struct State {
            std::string_view input;
            CommandSyntax syntax;
            std::vector<std::string> words{};
            std::string word{};
            std::size_t position{};
            std::size_t quote_offset{};
            std::size_t command_offset{};
            Quote quote = Quote::None;
            bool word_started = false;
            bool finished = false;
        };

        std::unexpected<CommandParseError> error(
            CommandParseErrorKind kind, std::size_t offset, std::string message) {
            return std::unexpected(CommandParseError{kind, offset, std::move(message)});
        }

        std::unexpected<CommandParseError> size_error() {
            return error(
                CommandParseErrorKind::TooLong, MAX_COMMAND_BYTES, "Command exceeds size limit");
        }

        bool newline(char value) {
            return value == '\r' || value == '\n';
        }

        bool invalid_control_character(char value) {
            const auto byte = static_cast<unsigned char>(value);
            return (byte < FIRST_PRINTABLE && value != '\t' && !newline(value)) ||
                   byte == DELETE_CHARACTER;
        }

        bool separator(char value, CommandSyntax syntax) {
            return value == '&' || value == '|' || newline(value) ||
                   (syntax == CommandSyntax::Posix && value == ';');
        }

        void finish_word(State& state) {
            if (state.word_started) {
                state.words.push_back(std::move(state.word));
                state.word.clear();
                state.word_started = false;
            }
        }

        std::expected<void, CommandParseError> escape(State& state) {
            const auto offset = state.position++;
            if (state.position == state.input.size()) {
                return error(CommandParseErrorKind::DanglingEscape,
                    offset,
                    "Escape has no following character");
            }
            if (!(state.position < MAX_COMMAND_BYTES)) {
                return size_error();
            }
            const auto next = state.input[state.position];
            if (state.syntax == CommandSyntax::Posix && state.quote == Quote::Double &&
                next != '$' && next != '`' && next != '"' && next != '\\' && next != '\n') {
                state.word.push_back('\\');
                state.word_started = true;
                return {};
            }
            if (next == '\n' || (state.syntax == CommandSyntax::Cmd && next == '\r')) {
                ++state.position;
                if (next == '\r' && state.position < state.input.size() &&
                    state.input[state.position] == '\n') {
                    if (!(state.position < MAX_COMMAND_BYTES)) {
                        return size_error();
                    }
                    ++state.position;
                }
                if (state.position == state.input.size()) {
                    return error(CommandParseErrorKind::DanglingEscape,
                        offset,
                        "Line continuation is incomplete");
                }
                return {};
            }
            if (invalid_control_character(next) || newline(next)) {
                return error(CommandParseErrorKind::InvalidCharacter,
                    state.position,
                    "Control character in command");
            }
            state.word.push_back(next);
            state.word_started = true;
            ++state.position;
            return {};
        }

        std::expected<void, CommandParseError> quoted(State& state, char value) {
            if (state.quote == Quote::Single) {
                if (value == '\'') {
                    state.quote = Quote::None;
                } else {
                    state.word.push_back(value);
                }
                ++state.position;
                return {};
            }
            if (value == '"') {
                state.quote = Quote::None;
                ++state.position;
            } else if (value == '\\' && state.syntax == CommandSyntax::Posix) {
                return escape(state);
            } else if (newline(value) && state.syntax == CommandSyntax::Cmd) {
                return error(CommandParseErrorKind::UnterminatedQuote,
                    state.quote_offset,
                    "Unterminated double quote");
            } else {
                state.word.push_back(value);
                ++state.position;
            }
            return {};
        }

        std::expected<void, CommandParseError> consume_unquoted(State& state, char value) {
            if (separator(value, state.syntax)) {
                if (newline(value) && state.words.empty() && !state.word_started) {
                    ++state.position;
                } else {
                    state.finished = true;
                }
                return {};
            }
            if (value == ' ' || value == '\t') {
                finish_word(state);
                ++state.position;
                return {};
            }
            if (state.syntax == CommandSyntax::Posix && value == '#' && !state.word_started) {
                if (!state.words.empty()) {
                    state.finished = true;
                } else {
                    while (state.position < state.input.size() &&
                           !newline(state.input[state.position]) &&
                           state.position < MAX_COMMAND_BYTES) {
                        ++state.position;
                    }
                }
                return {};
            }
            if (value == '<' || value == '>' || value == '(' || value == ')') {
                return error(CommandParseErrorKind::UnsupportedSyntax,
                    state.position,
                    "Redirection and command groups are not supported");
            }
            if (state.words.empty() && !state.word_started) {
                state.command_offset = state.position;
                if (state.syntax == CommandSyntax::Cmd && value == '@') {
                    ++state.position;
                    return {};
                }
            }
            if (value == '"' || (state.syntax == CommandSyntax::Posix && value == '\'')) {
                state.word_started = true;
                state.quote = value == '"' ? Quote::Double : Quote::Single;
                state.quote_offset = state.position++;
            } else if ((state.syntax == CommandSyntax::Posix && value == '\\') ||
                       (state.syntax == CommandSyntax::Cmd && value == '^')) {
                return escape(state);
            } else {
                state.word_started = true;
                state.word.push_back(value);
                ++state.position;
            }
            return {};
        }
        std::expected<void, CommandParseError> consume(State& state) {
            const auto value = state.input[state.position];
            if (invalid_control_character(value)) {
                return error(CommandParseErrorKind::InvalidCharacter,
                    state.position,
                    "Control character in command");
            }
            if (state.syntax == CommandSyntax::Posix && state.quote != Quote::Single &&
                (value == '`' || (value == '$' && state.position + 1 < state.input.size() &&
                                     state.input[state.position + 1] == '('))) {
                return error(CommandParseErrorKind::UnsupportedSyntax,
                    state.position,
                    "Command substitution is not supported");
            }
            if (state.quote != Quote::None) {
                return quoted(state, value);
            }
            return consume_unquoted(state, value);
        }
    } // namespace

    CommandParseResult CommandParser::parse(std::string_view input, CommandSyntax syntax) {
        if (syntax != CommandSyntax::Cmd && syntax != CommandSyntax::Posix) {
            return error(CommandParseErrorKind::InvalidSyntax, 0, "Unknown command syntax");
        }
        State state{input, syntax};
        while (state.position < input.size() && !state.finished) {
            if (!(state.position < MAX_COMMAND_BYTES) &&
                !(state.quote == Quote::None && separator(input[state.position], syntax) &&
                    (!state.words.empty() || state.word_started))) {
                return size_error();
            }
            auto result = consume(state);
            if (!result.has_value()) {
                return std::unexpected(std::move(result.error()));
            }
        }
        if (state.position > MAX_COMMAND_BYTES) {
            return size_error();
        }
        if (state.quote != Quote::None) {
            return error(
                CommandParseErrorKind::UnterminatedQuote, state.quote_offset, "Unterminated quote");
        }
        finish_word(state);
        if (state.words.empty()) {
            return error(CommandParseErrorKind::EmptyInput, 0, "No command in input");
        }
        if (state.words.front().empty()) {
            return error(
                CommandParseErrorKind::EmptyCommand, state.command_offset, "Command name is empty");
        }
        ParsedCommand command;
        command.name = std::move(state.words.front());
        for (std::size_t index = 1; index < state.words.size(); ++index) {
            command.arguments.push_back(std::move(state.words[index]));
        }
        return command;
    }
} // namespace enishi::bypass_shell
