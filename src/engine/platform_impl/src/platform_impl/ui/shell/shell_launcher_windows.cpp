#include "shell_launcher_windows.h"
#include <memory>
#include <vector>
#define NOMINMAX
#include <windows.h>
namespace enishi::platform_impl::native_shell_launcher {
    std::expected<void, std::string> launch(const std::filesystem::path& executable) {
        const auto path = executable.wstring();
        const auto command = L"\"" + path + L"\"";
        std::vector<wchar_t> command_line(command.begin(), command.end());
        command_line.push_back(L'\0');
        STARTUPINFOW startup{};
        startup.cb = sizeof(startup);
        PROCESS_INFORMATION info{};
        if (CreateProcessW(path.c_str(),
                command_line.data(),
                nullptr,
                nullptr,
                FALSE,
                CREATE_NEW_CONSOLE,
                nullptr,
                nullptr,
                &startup,
                &info) == FALSE) {
            return std::unexpected(
                "Cannot open shell terminal (OS error " + std::to_string(GetLastError()) + ")");
        }
        const std::unique_ptr<void, decltype(&CloseHandle)> process(info.hProcess, CloseHandle);
        const std::unique_ptr<void, decltype(&CloseHandle)> thread(info.hThread, CloseHandle);
        return {};
    }
} // namespace enishi::platform_impl::native_shell_launcher
