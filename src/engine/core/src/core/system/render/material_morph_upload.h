#pragma once
#include <component/model_component.h>
#include <component/morph_component.h>
#include <engine_types/renderer/uniform_buffer/material.h>
#include <platform/renderer/interface_renderer.h>

namespace enishi::core {
    [[nodiscard]] platform::RenderResult<void> write_material_uniform(
        platform::IResourceUpdater& updater, const types::UniformMaterial& material);
    [[nodiscard]] platform::RenderResult<void> upload_material_morphs(platform::IRenderer& renderer,
        const component::ModelComponent& model,
        const component::MorphComponent& morph);
} // namespace enishi::core
