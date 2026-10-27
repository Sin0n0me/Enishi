#include "command_parser_property_tests.h"
#include "parser_test_support.h"
#include <random>

namespace enishi::tests {
    namespace {
        using bypass_shell::CommandParser;
        using bypass_shell::CommandSyntax;
        constexpr unsigned RANDOM_SEED = 0x45A17;
        constexpr std::size_t ROUND_TRIPS = 4000;
        constexpr std::size_t FUZZ_CASES = 20000;
        constexpr unsigned FIRST_PRINTABLE = 32;
        constexpr unsigned PRINTABLE_END = 127;
        constexpr unsigned ASCII_END = 128;
        constexpr unsigned MAX_WORD_BYTES = 128;
        constexpr unsigned MAX_ARGUMENTS = 8;
        constexpr unsigned MAX_FUZZ_BYTES = 256;

        std::string encode(std::string_view word, CommandSyntax syntax) {
            if (syntax == CommandSyntax::Posix) {
                std::string result = "'";
                for (const char byte : word) {
                    if (byte == '\'') {
                        result += "'\\''";
                    } else {
                        result += byte;
                    }
                }
                return result + "'";
            }
            if (word.empty()) {
                return "\"\"";
            }
            std::string result;
            for (const char byte : word) {
                // Escaping every character independently avoids relying on quote-state logic.
                result += '^';
                result += byte;
            }
            return result;
        }

        void printable_pairs() {
            for (const auto syntax : {CommandSyntax::Cmd, CommandSyntax::Posix}) {
                for (unsigned first = FIRST_PRINTABLE; first < PRINTABLE_END; ++first) {
                    for (unsigned second = FIRST_PRINTABLE; second < PRINTABLE_END; ++second) {
                        const std::string word{static_cast<char>(first), static_cast<char>(second)};
                        expect_command("tool " + encode(word, syntax), syntax, "tool", {word});
                    }
                }
            }
        }

        std::string random_word(std::mt19937& random) {
            const auto length = random() % MAX_WORD_BYTES;
            std::string word;
            for (std::size_t index = 0; index < length; ++index) {
                // Map the random integer to [FIRST_PRINTABLE, PRINTABLE_END).
                word += static_cast<char>(
                    FIRST_PRINTABLE + random() % (PRINTABLE_END - FIRST_PRINTABLE));
            }
            return word;
        }

        void round_trips() {
            std::mt19937 random(RANDOM_SEED);
            for (const auto syntax : {CommandSyntax::Cmd, CommandSyntax::Posix}) {
                for (std::size_t iteration = 0; iteration < ROUND_TRIPS; ++iteration) {
                    const auto name = "tool" + random_word(random);
                    std::string input = encode(name, syntax);
                    std::vector<std::string> arguments;
                    const auto count = random() % MAX_ARGUMENTS;
                    for (unsigned index = 0; index < count; ++index) {
                        arguments.push_back(random_word(random));
                        input += " " + encode(arguments.back(), syntax);
                    }
                    expect_command(input, syntax, name, arguments);
                    input += "&&";
                    for (unsigned index = 0; index < MAX_FUZZ_BYTES; ++index) {
                        input += static_cast<char>(random() % ASCII_END);
                    }
                    expect_command(input, syntax, name, arguments);
                }
                const std::vector<std::string> unicode{
                    "\u65e5\u672c\u8a9e", "\u9854\U0001f600", "\u7a7a \u767d", "\t", ""};
                std::string input = encode("\u5b9f\u884c", syntax);
                for (const auto& word : unicode) {
                    input += " " + encode(word, syntax);
                }
                expect_command(input, syntax, "\u5b9f\u884c", unicode);
            }
        }

        void arbitrary_inputs() {
            std::mt19937 random(RANDOM_SEED);
            for (const auto syntax : {CommandSyntax::Cmd, CommandSyntax::Posix}) {
                for (std::size_t iteration = 0; iteration < FUZZ_CASES; ++iteration) {
                    std::string input;
                    const auto length = random() % MAX_FUZZ_BYTES;
                    for (unsigned index = 0; index < length; ++index) {
                        input += static_cast<char>(random() % ASCII_END);
                    }
                    const auto result = CommandParser::parse(input, syntax);
                    const auto repeated = CommandParser::parse(input, syntax);
                    check(result.has_value() == repeated.has_value(),
                        "Parsing must be deterministic");
                    if (result.has_value()) {
                        check(!result->name.empty(), "Successful command must have a name");
                        check(result->name == repeated->name &&
                                  result->arguments == repeated->arguments,
                            "Repeated parsing must produce identical tokens");
                    } else {
                        check(
                            !(result.error().offset > input.size()), "Error offset exceeds input");
                        check(
                            !result.error().message.empty(), "Error diagnostic must not be empty");
                        check(result.error().kind == repeated.error().kind &&
                                  result.error().offset == repeated.error().offset &&
                                  result.error().message == repeated.error().message,
                            "Repeated parsing must produce identical errors");
                    }
                }
            }
        }
    } // namespace
    void command_parser_property_tests() {
        std::fprintf(stderr,
            "Property seed=%u; round trips=%zu/mode; fuzz=%zu/mode\n",
            RANDOM_SEED,
            ROUND_TRIPS,
            FUZZ_CASES);
        run_test("Exhaustive printable ASCII pairs", printable_pairs);
        run_test("Generated argument round trips and suffix independence", round_trips);
        run_test("Deterministic arbitrary input safety", arbitrary_inputs);
    }
} // namespace enishi::tests
