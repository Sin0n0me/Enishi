#include "windows_terminal_input.h"
#include <algorithm>
#include <array>
#define NOMINMAX
#include <windows.h>

namespace enishi::platform_impl {
    namespace {
        constexpr std::size_t INPUT_CHUNK = 4096;
        constexpr std::size_t MAX_INPUT_CHARACTERS = 16384;
        void echo(std::wstring_view text) {
            DWORD written{};
            WriteConsoleW(GetStdHandle(STD_OUTPUT_HANDLE),
                text.data(),
                static_cast<DWORD>(text.size()),
                &written,
                nullptr);
        }
    } // namespace
    struct WindowsTerminalInput::Impl {
        HANDLE input = GetStdHandle(STD_INPUT_HANDLE);
        bool console = false;
        std::wstring line;
        inline Impl() {
            DWORD mode{};
            this->console = GetConsoleMode(this->input, &mode) != FALSE;
        }
        inline std::expected<std::optional<std::string>, std::string> read_console() {
            DWORD available{};
            if (GetNumberOfConsoleInputEvents(this->input, &available) == FALSE) {
                return std::unexpected("Cannot poll console input");
            }
            constexpr DWORD MAX_EVENTS_PER_POLL = 128;
            available = (std::min)(available, MAX_EVENTS_PER_POLL);
            for (DWORD index = 0; index < available; ++index) {
                INPUT_RECORD event{};
                DWORD count{};
                if (ReadConsoleInputW(this->input, &event, 1, &count) == FALSE) {
                    return std::unexpected("Cannot read console input");
                }
                if (event.EventType != KEY_EVENT || event.Event.KeyEvent.bKeyDown == FALSE) {
                    continue;
                }
                const auto character = event.Event.KeyEvent.uChar.UnicodeChar;
                constexpr wchar_t END_OF_INPUT = 26;
                if (character == END_OF_INPUT && this->line.empty()) {
                    return std::string{};
                }
                if (character == L'\r') {
                    echo(L"\r\n");
                    this->line.append(L"\r\n");
                    const auto size = WideCharToMultiByte(GetConsoleCP(),
                        0,
                        this->line.data(),
                        static_cast<int>(this->line.size()),
                        nullptr,
                        0,
                        nullptr,
                        nullptr);
                    if (size == 0) {
                        return std::unexpected("Cannot encode console input");
                    }
                    std::string result(static_cast<std::size_t>(size), '\0');
                    WideCharToMultiByte(GetConsoleCP(),
                        0,
                        this->line.data(),
                        static_cast<int>(this->line.size()),
                        result.data(),
                        size,
                        nullptr,
                        nullptr);
                    this->line.clear();
                    return result;
                }
                for (WORD repeat = 0; repeat < event.Event.KeyEvent.wRepeatCount; ++repeat) {
                    if (character == L'\b') {
                        if (!this->line.empty()) {
                            this->line.pop_back();
                            echo(L"\b \b");
                        }
                    } else if (character != 0) {
                        if (!(this->line.size() < MAX_INPUT_CHARACTERS)) {
                            return std::unexpected("Console command is too long");
                        }
                        this->line.push_back(character);
                        echo(std::wstring_view(&character, 1));
                    }
                }
            }
            return std::optional<std::string>{};
        }
    };
    WindowsTerminalInput::WindowsTerminalInput()
        : impl(std::make_unique<Impl>()) {
    }
    WindowsTerminalInput::~WindowsTerminalInput() = default;
    std::expected<std::optional<std::string>, std::string> WindowsTerminalInput::read() {
        std::array<char, INPUT_CHUNK> buffer{};
        if (this->impl->console) {
            return this->impl->read_console();
        }
        if (GetFileType(this->impl->input) == FILE_TYPE_PIPE) {
            DWORD available{};
            if (PeekNamedPipe(this->impl->input, nullptr, 0, nullptr, &available, nullptr) ==
                FALSE) {
                if (GetLastError() == ERROR_BROKEN_PIPE) {
                    return std::string{};
                }
                return std::unexpected("Cannot poll redirected input");
            }
            if (available == 0) {
                return std::optional<std::string>{};
            }
        }
        DWORD count{};
        if (ReadFile(this->impl->input,
                buffer.data(),
                static_cast<DWORD>(buffer.size()),
                &count,
                nullptr) == FALSE) {
            if (GetLastError() == ERROR_BROKEN_PIPE) {
                return std::string{};
            }
            return std::unexpected("Cannot read redirected input");
        }
        return std::string(buffer.data(), static_cast<std::size_t>(count));
    }
} // namespace enishi::platform_impl
