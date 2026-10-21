#pragma once
#include <engine_types/handle/renderer/handles/mesh_handles.h>
#include <engine_types/renderer/uniform_buffer/material.h>
#include <foundation/result/result.h>
#include <platform/renderer/updater/interface_resource_updater.h>
#include <span>
#include <vector>

namespace enishi::renderer {
    [[nodiscard]] foundation::Result<void, platform::RenderError> write_bone_matrices(
        platform::IResourceUpdater& updater, std::span<const glm::mat4> matrices);
    [[nodiscard]] foundation::Result<void, platform::RenderError> write_vertex_positions(
        platform::IResourceUpdater& updater,
        std::size_t offset,
        std::span<const glm::vec3> positions);
    [[nodiscard]] foundation::Result<void, platform::RenderError> write_material_uniform(
        platform::IResourceUpdater& updater, const types::UniformMaterial& material);
    [[nodiscard]] foundation::Result<void, platform::RenderError> stage_vertex_uvs(
        platform::IResourceUpdater& updater,
        std::span<const types::MeshHandles::UVStream> streams,
        std::span<const std::vector<glm::vec4>> channels);
} // namespace enishi::renderer
