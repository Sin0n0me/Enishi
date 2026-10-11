#pragma once
#include <foundation/result/result.h>

namespace enishi::sub_system {
    enum class UIError { InitializationFailed };

    class IUIBackend {
      public:
        virtual ~IUIBackend(void) noexcept = default;
        // The UI context must be current during all backend calls.
        [[nodiscard]] virtual foundation::VoidResult<UIError> init(void) = 0;
        virtual void shutdown(void) noexcept = 0;
        virtual void begin_frame(void) = 0;
        virtual void render(void) = 0;
    };
} // namespace enishi::sub_system
