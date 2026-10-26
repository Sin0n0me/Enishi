#include "shell_process.h"
#include <utility>

namespace enishi::bypass_shell {
    ShellProcess::ShellProcess(std::unique_ptr<platform::IShellProcess> process)
        : process(std::move(process)) {
    }
    ShellProcess::~ShellProcess() = default;
    ShellResult<void> ShellProcess::start() {
        return this->process->start();
    }
    ShellResult<void> ShellProcess::write(std::string_view data) {
        return this->process->write(data);
    }
    void ShellProcess::close_input() {
        this->process->close_input();
    }
    void ShellProcess::terminate() {
        this->process->terminate();
    }
    ShellResult<std::string> ShellProcess::read(ShellEventKind stream) {
        switch (stream) {
            case ShellEventKind::Stdout:
                return this->process->read(platform::ProcessOutput::Stdout);
            case ShellEventKind::Stderr:
                return this->process->read(platform::ProcessOutput::Stderr);
            default:
                return std::unexpected("Invalid shell output stream");
        }
    }
    ShellResult<std::optional<int>> ShellProcess::exit_code() {
        return this->process->exit_code();
    }
} // namespace enishi::bypass_shell
