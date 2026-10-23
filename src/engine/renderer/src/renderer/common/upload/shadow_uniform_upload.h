#pragma once
#include <engine_types/renderer/uniform_buffer/shadow.h>
#include <platform/renderer/interface_renderer.h>
namespace enishi::renderer {
    [[nodiscard]] platform::RenderResult<void> upload_model_shadow(platform::IRenderer& renderer,
        types::RenderHandle mesh_handle,
        const types::UniformShadow& shadow);
}
