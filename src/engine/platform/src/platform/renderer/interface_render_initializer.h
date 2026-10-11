#pragma once
#include "../errors/renderer_errors.h"
#include "../window/window_handle.h"
#include <engine_types/handle/renderer/render_handle.h>
#include <engine_types/window/window_types.h>
#include <foundation/result/result.h>
#include <memory>
#include <optional>

namespace enishi::platform {
    // 各プラットフォームに応じたウィンドウハンドルを受け取って初期化を行う実装を行う
    // IRendererを返すことでIRendererはプラットフォームに応じたウィンドウを知らずに初期化ができる
    template <typename T> class IRendererInitializer {
      public:
        virtual ~IRendererInitializer(void) noexcept = default;

        virtual foundation::Result<std::shared_ptr<T>, RenderError> init(
            const WindowHandle& window_handle, const types::WindowSize& window_size) = 0;
    };
} // namespace enishi::platform