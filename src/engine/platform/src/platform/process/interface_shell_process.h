#pragma once
#include <expected>
#include <optional>
#include <string>
#include <string_view>

namespace enishi::platform {
    template <typename T> using ProcessResult = std::expected<T, std::string>;
    enum class ProcessOutput { Stdout, Stderr };

    class IShellProcess {
      public:
        virtual ~IShellProcess() = default;
        virtual ProcessResult<void> start() = 0;
        virtual ProcessResult<void> write(std::string_view data) = 0;
        virtual void close_input() = 0;
        virtual void terminate() = 0;
        // One reader per stream; an empty string means no bytes are currently available.
        virtual ProcessResult<std::string> read(ProcessOutput stream) = 0;
        virtual ProcessResult<std::optional<int>> exit_code() = 0;
    };
} // namespace enishi::platform
