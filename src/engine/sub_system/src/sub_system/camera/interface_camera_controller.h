#pragma once
#include <engine_types/renderer/uniform_buffer/camera.h>
#include <foundation/result/result.h>

namespace enishi::sub_system {
    enum class CameraError { InvalidMatrix };

    // Call on the application thread before frame updates. Returned snapshots are independent
    // values; edits must be submitted through set_view_projection to keep MVP consistent.
    class ICameraController {
      public:
        virtual ~ICameraController() noexcept = default;
        [[nodiscard]] virtual types::UniformCamera get_camera() const = 0;
        [[nodiscard]] virtual foundation::VoidResult<CameraError> set_view_projection(
            const glm::mat4& view, const glm::mat4& projection) = 0;
        virtual void reset() = 0;
    };
} // namespace enishi::sub_system
