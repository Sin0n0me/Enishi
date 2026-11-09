#include "windows_console_input.h"
#include <array>
#include <atomic>
#include <chrono>
#include <mutex>
#include <thread>
#include <utility>
#define NOMINMAX
#include <windows.h>
namespace enishi::platform_impl {
    bool has_console_terminal() {
        DWORD input_mode{};
        DWORD output_mode{};
        return GetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), &input_mode) != FALSE &&
               GetConsoleMode(GetStdHandle(STD_OUTPUT_HANDLE), &output_mode) != FALSE;
    }
    struct WindowsConsoleInput::Impl {
        HANDLE input = GetStdHandle(STD_INPUT_HANDLE);
        HANDLE output = GetStdHandle(STD_OUTPUT_HANDLE);
        DWORD input_mode{};
        DWORD output_mode{};
        UINT input_page = GetConsoleCP();
        UINT output_page = GetConsoleOutputCP();
        bool restore = false;
        bool eof = false;
        std::string error;
        std::string pending;
        std::mutex mutex;
        std::unique_ptr<void, decltype(&CloseHandle)> reader_handle{nullptr, CloseHandle};
        std::atomic<bool> finished{false};
        std::jthread reader;
        Impl();
        ~Impl();
        void read_console(std::stop_token stop);
    };
    WindowsConsoleInput::Impl::Impl() {
        if (GetConsoleMode(this->input, &this->input_mode) == FALSE ||
            GetConsoleMode(this->output, &this->output_mode) == FALSE) {
            this->error = "Cannot read console modes";
            return;
        }
        this->restore = true;
        const DWORD raw_input =
            (this->input_mode & ~(ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT | ENABLE_PROCESSED_INPUT)) |
            ENABLE_VIRTUAL_TERMINAL_INPUT;
        if (SetConsoleMode(this->input, raw_input) == FALSE ||
            SetConsoleMode(this->output,
                this->output_mode | ENABLE_PROCESSED_OUTPUT | ENABLE_VIRTUAL_TERMINAL_PROCESSING) ==
                FALSE ||
            SetConsoleCP(CP_UTF8) == FALSE || SetConsoleOutputCP(CP_UTF8) == FALSE) {
            this->error = "Cannot enable virtual terminal input/output";
            return;
        }
        // A parent created in a new process group may pass down the Ctrl+C ignore flag.
        // Clear it before spawning the child; raw console input still forwards the key.
        if (SetConsoleCtrlHandler(nullptr, FALSE) == FALSE) {
            this->error = "Cannot enable child console interrupts";
            return;
        }
        this->reader = std::jthread([this](std::stop_token stop) {
            this->read_console(stop);
            this->finished.store(true);
        });
    }
    WindowsConsoleInput::Impl::~Impl() {
        if (this->reader.joinable()) {
            this->reader.request_stop();
            // Cancellation may race the next ReadFile. Repeat until the reader has exited.
            constexpr auto CANCEL_INTERVAL = std::chrono::milliseconds(1);
            while (!this->finished.load()) {
                {
                    std::lock_guard lock(this->mutex);
                    if (this->reader_handle != nullptr) {
                        CancelSynchronousIo(this->reader_handle.get());
                    }
                }
                std::this_thread::sleep_for(CANCEL_INTERVAL);
            }
            this->reader.join();
        }
        if (this->restore) {
            SetConsoleMode(this->input, this->input_mode);
            SetConsoleMode(this->output, this->output_mode);
            SetConsoleCP(this->input_page);
            SetConsoleOutputCP(this->output_page);
        }
    }
    void WindowsConsoleInput::Impl::read_console(std::stop_token stop) {
        {
            std::lock_guard lock(this->mutex);
            HANDLE thread = nullptr;
            if (DuplicateHandle(GetCurrentProcess(),
                    GetCurrentThread(),
                    GetCurrentProcess(),
                    &thread,
                    0,
                    FALSE,
                    DUPLICATE_SAME_ACCESS) == FALSE) {
                this->error = "Cannot prepare console input cancellation";
                return;
            }
            this->reader_handle.reset(thread);
        }
        constexpr std::size_t INPUT_CHUNK = 4096;
        constexpr std::size_t MAX_PENDING_BYTES = 65536;
        std::array<char, INPUT_CHUNK> bytes{};
        while (!stop.stop_requested()) {
            DWORD count{};
            const bool success =
                ReadFile(
                    this->input, bytes.data(), static_cast<DWORD>(bytes.size()), &count, nullptr) !=
                FALSE;
            if (stop.stop_requested()) {
                return;
            }
            std::lock_guard lock(this->mutex);
            if (!success) {
                this->error = "Cannot read virtual terminal input";
                return;
            }
            if (count == 0) {
                this->eof = true;
                return;
            }
            if (count > MAX_PENDING_BYTES - this->pending.size()) {
                this->error = "Terminal input buffer exceeded its limit";
                return;
            }
            this->pending.append(bytes.data(), count);
        }
    }
    WindowsConsoleInput::WindowsConsoleInput()
        : impl(std::make_unique<Impl>()) {
    }
    WindowsConsoleInput::~WindowsConsoleInput() = default;
    std::expected<std::optional<std::string>, std::string> WindowsConsoleInput::read() {
        std::lock_guard lock(this->impl->mutex);
        if (!this->impl->error.empty()) {
            return std::unexpected(this->impl->error);
        }
        if (!this->impl->pending.empty()) {
            return std::exchange(this->impl->pending, {});
        }
        if (this->impl->eof) {
            return std::string{};
        }
        return std::optional<std::string>{};
    }
} // namespace enishi::platform_impl
