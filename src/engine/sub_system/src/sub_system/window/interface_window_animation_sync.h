#pragma once
#include <engine_types/assets/model/addons/bone.h>
#include <engine_types/handle/handle_type.h>
#include <foundation/result/result.h>

namespace enishi::sub_system {
    enum class WindowAnimationMode { ReplaceModelMovement, AddWindowMovement };
    enum class WindowAnimationError { InvalidTarget };

    // One desktop window follows one final model-space bone position at a time.
    // Call on the application thread before update. The next valid frame establishes the
    // baseline; only subsequent X/Y motion is followed. No target is selected by default.
    class IWindowAnimationSync {
      public:
        virtual ~IWindowAnimationSync() noexcept = default;
        [[nodiscard]] virtual foundation::VoidResult<WindowAnimationError> follow(
            types::HandleId model, types::BoneIndex bone, WindowAnimationMode mode) = 0;
        // Switching mode/target or stopping clears display compensation without moving the
        // window back. Replace mode shifts the entire window's camera, including other models.
        virtual void stop() = 0;
    };
} // namespace enishi::sub_system
