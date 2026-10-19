#pragma once
#include <ecs/registry.h>
#include <engine_types/renderer/uniform_buffer/camera.h>
#include <platform/renderer/interface_renderer.h>

namespace enishi::core {
    foundation::Result<void, platform::RenderError> upload_shadow(
        platform::IRenderer& renderer, ecs::Registry& registry, const types::UniformCamera& camera);
}
