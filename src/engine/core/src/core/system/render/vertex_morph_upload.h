#pragma once
#include <component/model_component.h>
#include <component/morph_component.h>
#include <platform/renderer/interface_renderer.h>
#include <span>

namespace enishi::core {
    [[nodiscard]] platform::RenderResult<void> write_vertex_positions(
        platform::IResourceUpdater& updater,
        std::size_t offset,
        std::span<const glm::vec3> positions);
    [[nodiscard]] platform::RenderResult<void> upload_vertex_morphs(platform::IRenderer& renderer,
        const component::ModelComponent& model,
        const component::MorphComponent& morph);
} // namespace enishi::core
