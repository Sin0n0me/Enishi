#pragma once
#include <component/model_component.h>
#include <engine_types/renderer/uniform_buffer/camera.h>
#include <platform/renderer/interface_renderer.h>

namespace enishi::core {
    [[nodiscard]] foundation::Result<void, platform::RenderError> write_camera(
        platform::IResourceUpdater& updater, const types::UniformCamera& camera);
    [[nodiscard]] foundation::Result<void, platform::RenderError> upload_camera(
        platform::IRenderer& renderer,
        const component::ModelComponent& model,
        const types::UniformCamera& camera);
} // namespace enishi::core
