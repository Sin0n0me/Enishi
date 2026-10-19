#pragma once
#include <animation/uv_morph.h>
#include <engine_types/handle/renderer/handles/mesh_handles.h>
#include <platform/renderer/updater/interface_resource_updater.h>

namespace enishi::core {
    // Stage UV changes before the caller uploads the combined vertex attributes.
    [[nodiscard]] foundation::Result<void, platform::RenderError> stage_vertex_uvs(
        platform::IResourceUpdater& updater,
        std::span<const types::MeshHandles::UVStream> streams,
        const animation::UVChannels& channels);
} // namespace enishi::core
