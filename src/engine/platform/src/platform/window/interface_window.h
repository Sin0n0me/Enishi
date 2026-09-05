#pragma once
#include "../errors/window_errors.h"
#include "../input/interface_input.h"
#include "window_handle.h"
#include <engine_types/window/window_types.h>
#include <foundation/option/option.h>
#include <foundation/result/result.h>
#include <string>

namespace enishi::platform {
    class IWindow {
      public:
        [[nodiscard]] virtual foundation::VoidResult<WindowError> init(void) noexcept = 0;

        [[nodiscard]] virtual foundation::Option<WindowHandle> get_handle(void) const noexcept = 0;

        [[nodiscard]] virtual foundation::Option<const IInput*> get_input(void) const noexcept = 0;

        [[nodiscard]] virtual foundation::Option<types::WindowPosition> get_position(
            void) const noexcept = 0;

        [[nodiscard]] virtual foundation::VoidResult<WindowError> set_position(
            const types::WindowPosition& position) noexcept = 0;

        [[nodiscard]] virtual foundation::Option<types::WindowSize> get_size(
            void) const noexcept = 0;

        [[nodiscard]] virtual foundation::VoidResult<WindowError> set_size(
            const types::WindowSize& size) noexcept = 0;

        [[nodiscard]] virtual foundation::VoidResult<WindowError> set_title(
            const std::string& title) noexcept = 0;

        [[nodiscard]] virtual foundation::Option<std::string> get_title(void) const noexcept = 0;

        // プログラム側から閉じる場合
        virtual void close(void) = 0;

        [[nodiscard]] virtual bool should_close(void) const = 0;

        virtual void poll_events(void) = 0;
    };
} // namespace enishi::platform