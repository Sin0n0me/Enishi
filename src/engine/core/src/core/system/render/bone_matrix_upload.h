#pragma once
#include <component/model_component.h>
#include <component/skinning_component.h>
#include <platform/renderer/interface_renderer.h>

namespace enishi::core {
    [[nodiscard]] foundation::Result<void, platform::RenderError> write_bone_matrices(
        platform::IResourceUpdater& updater, std::span<const glm::mat4> matrices);
    [[nodiscard]] foundation::Result<void, platform::RenderError> upload_bone_matrices(
        platform::IRenderer& renderer,
        const component::ModelComponent& model,
        const component::SkinningComponent& skinning);
} // namespace enishi::core
