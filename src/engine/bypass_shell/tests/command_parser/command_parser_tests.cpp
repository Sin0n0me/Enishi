#include "command_parser_tests.h"
#include "command_parser_property_tests.h"
#include "parser_test_support.h"

namespace enishi::tests {
    namespace {
        using bypass_shell::CommandParseErrorKind;
        using bypass_shell::CommandSyntax;
        constexpr std::size_t COMMAND_BYTE_LIMIT = 65536;

        void posix_words() {
            expect_command(
                " git\tstatus --short\r\n", CommandSyntax::Posix, "git", {"status", "--short"});
            expect_command(
                R"(echo "a b" '' ab"cd"'ef')", CommandSyntax::Posix, "echo", {"a b", "", "abcdef"});
            expect_command(R"(echo "" '')", CommandSyntax::Posix, "echo", {"", ""});
            expect_command(R"(echo a\ b \&)", CommandSyntax::Posix, "echo", {"a b", "&"});
            expect_command(R"(echo "a\qb")", CommandSyntax::Posix, "echo", {"a\\qb"});
            expect_command(R"(echo '$(x)' '$HOME' '`x`')",
                CommandSyntax::Posix,
                "echo",
                {"$(x)", "$HOME", "`x`"});
            expect_command("echo $HOME *.txt", CommandSyntax::Posix, "echo", {"$HOME", "*.txt"});
            expect_command(R"(echo a\;b)", CommandSyntax::Posix, "echo", {"a;b"});
            expect_command("echo 'a\nb'", CommandSyntax::Posix, "echo", {"a\nb"});
            expect_command("echo \"a\nb\"", CommandSyntax::Posix, "echo", {"a\nb"});
            expect_command("# comment\n git status", CommandSyntax::Posix, "git", {"status"});
            expect_command("echo x # ignore && next", CommandSyntax::Posix, "echo", {"x"});
            expect_command(R"(echo '#' x#y)", CommandSyntax::Posix, "echo", {"#", "x#y"});
            expect_command("@echo ok", CommandSyntax::Posix, "@echo", {"ok"});
            expect_command("echo a\\\nb", CommandSyntax::Posix, "echo", {"ab"});
            expect_command("\\\necho x", CommandSyntax::Posix, "echo", {"x"});
            expect_command("echo \\\n x", CommandSyntax::Posix, "echo", {"x"});
            expect_command("echo \"a\\\nb\"", CommandSyntax::Posix, "echo", {"ab"});
        }
        void cmd_words() {
            expect_command(R"("C:\Program Files\app.exe" "x y" C:\tmp\file)",
                CommandSyntax::Cmd,
                "C:\\Program Files\\app.exe",
                {"x y", "C:\\tmp\\file"});
            expect_command("echo 'two words'", CommandSyntax::Cmd, "echo", {"'two", "words'"});
            expect_command(
                "echo %TEMP% !VALUE!", CommandSyntax::Cmd, "echo", {"%TEMP%", "!VALUE!"});
            expect_command(R"(echo "^&" ^| ^")", CommandSyntax::Cmd, "echo", {"^&", "|", "\""});
            expect_command("echo ^^ ^&^& ^< ^> ^( ^)",
                CommandSyntax::Cmd,
                "echo",
                {"^", "&&", "<", ">", "(", ")"});
            expect_command("echo a^\r\nb", CommandSyntax::Cmd, "echo", {"ab"});
            expect_command("echo ^\n x", CommandSyntax::Cmd, "echo", {"x"});
            expect_command("@echo ok", CommandSyntax::Cmd, "echo", {"ok"});
            expect_command("echo #tag", CommandSyntax::Cmd, "echo", {"#tag"});
            expect_command("echo one;two", CommandSyntax::Cmd, "echo", {"one;two"});
            expect_command(R"(echo "" a" b"c)", CommandSyntax::Cmd, "echo", {"", "a bc"});
            expect_command(R"("@tool" arg)", CommandSyntax::Cmd, "@tool", {"arg"});
        }
        void first_command_only() {
            const std::vector<std::string> tails{"next",
                "'unterminated",
                "$(broken",
                " > & ",
                std::string("nul\0byte", 8),
                std::string(COMMAND_BYTE_LIMIT * 2, 'x')};
            for (const auto syntax : {CommandSyntax::Cmd, CommandSyntax::Posix}) {
                for (const auto* delimiter : {" & ", " && ", " | ", " || ", "\n", "\r\n"}) {
                    for (const auto& tail : tails) {
                        expect_command(std::string("echo first") + delimiter + tail,
                            syntax,
                            "echo",
                            {"first"});
                    }
                }
                expect_command(R"(echo "a && b | c")", syntax, "echo", {"a && b | c"});
                expect_command("  \r\n\techo first\nsecond", syntax, "echo", {"first"});
            }
            for (const auto& tail : tails) {
                expect_command("echo first;" + tail, CommandSyntax::Posix, "echo", {"first"});
            }
        }
        void errors() {
            for (const auto syntax : {CommandSyntax::Cmd, CommandSyntax::Posix}) {
                expect_error(" \t\n", syntax, CommandParseErrorKind::EmptyInput, 0);
                expect_error("", syntax, CommandParseErrorKind::EmptyInput, 0);
                expect_error("&& next", syntax, CommandParseErrorKind::EmptyInput, 0);
                expect_error("\"\" hi", syntax, CommandParseErrorKind::EmptyCommand, 0);
                expect_error("echo \"x", syntax, CommandParseErrorKind::UnterminatedQuote, 5);
                for (const auto* input :
                    {"echo x >file", "echo x <file", "echo x (bad)", "echo x )"}) {
                    expect_error(input, syntax, CommandParseErrorKind::UnsupportedSyntax, 7);
                }
            }
            expect_error(
                R"(echo \)", CommandSyntax::Posix, CommandParseErrorKind::DanglingEscape, 5);
            expect_error("echo ^", CommandSyntax::Cmd, CommandParseErrorKind::DanglingEscape, 5);
            expect_error(
                "echo \\\n", CommandSyntax::Posix, CommandParseErrorKind::DanglingEscape, 5);
            expect_error(
                "echo ^\r\n", CommandSyntax::Cmd, CommandParseErrorKind::DanglingEscape, 5);
            expect_error(
                "echo 'x", CommandSyntax::Posix, CommandParseErrorKind::UnterminatedQuote, 5);
            expect_error(
                "echo \"x\ny\"", CommandSyntax::Cmd, CommandParseErrorKind::UnterminatedQuote, 5);
            expect_error(R"cmd(echo "$(id)")cmd",
                CommandSyntax::Posix,
                CommandParseErrorKind::UnsupportedSyntax,
                6);
            expect_error(
                "echo `id`", CommandSyntax::Posix, CommandParseErrorKind::UnsupportedSyntax, 5);
            expect_error(
                "x", static_cast<CommandSyntax>(-1), CommandParseErrorKind::InvalidSyntax, 0);
        }
        void control_characters() {
            constexpr unsigned CONTROL_END = 32;
            constexpr unsigned DELETE_CHARACTER = 127;
            constexpr std::size_t ARGUMENT_OFFSET = 5;
            for (const auto syntax : {CommandSyntax::Cmd, CommandSyntax::Posix}) {
                for (unsigned value = 0; value < CONTROL_END; ++value) {
                    const auto byte = static_cast<char>(value);
                    if (byte == '\t' || byte == '\r' || byte == '\n') {
                        continue;
                    }
                    for (const auto& prefix : {std::string("echo "), std::string("echo \"")}) {
                        expect_error(prefix + byte + "tail",
                            syntax,
                            CommandParseErrorKind::InvalidCharacter,
                            prefix.size());
                    }
                    const std::string escape = syntax == CommandSyntax::Cmd ? "^" : "\\";
                    expect_error("echo " + escape + byte,
                        syntax,
                        CommandParseErrorKind::InvalidCharacter,
                        ARGUMENT_OFFSET + escape.size());
                }
                expect_error(std::string("echo ") + static_cast<char>(DELETE_CHARACTER),
                    syntax,
                    CommandParseErrorKind::InvalidCharacter,
                    ARGUMENT_OFFSET);
            }
        }
        void size_limits() {
            for (const auto syntax : {CommandSyntax::Cmd, CommandSyntax::Posix}) {
                const std::string name(COMMAND_BYTE_LIMIT, 'x');
                expect_command(name, syntax, name, {});
                expect_command(name + "&& ignored", syntax, name, {});
                expect_command(name.substr(1), syntax, name.substr(1), {});
                expect_error(
                    name + "x", syntax, CommandParseErrorKind::TooLong, COMMAND_BYTE_LIMIT);
                const auto escape = syntax == CommandSyntax::Cmd ? '^' : '\\';
                expect_error(std::string(COMMAND_BYTE_LIMIT - 1, 'x') + escape + "x",
                    syntax,
                    CommandParseErrorKind::TooLong,
                    COMMAND_BYTE_LIMIT);
                expect_command("echo ok && " + name + name, syntax, "echo", {"ok"});
            }
        }
    } // namespace
} // namespace enishi::tests
int main() {
    enishi::tests::run_test("POSIX names, quotes and escapes", enishi::tests::posix_words);
    enishi::tests::run_test("cmd names, quotes and escapes", enishi::tests::cmd_words);
    enishi::tests::run_test("Ignore all subsequent commands", enishi::tests::first_command_only);
    enishi::tests::run_test("Error categories and byte offsets", enishi::tests::errors);
    enishi::tests::run_test("Control character rejection", enishi::tests::control_characters);
    enishi::tests::run_test("Command size boundaries", enishi::tests::size_limits);
    enishi::tests::command_parser_property_tests();
}
