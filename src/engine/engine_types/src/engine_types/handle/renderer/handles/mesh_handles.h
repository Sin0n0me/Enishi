#pragma once
#include <engine_types/handle/renderer/render_handle.h>
#include <vector>

namespace enishi::types {
    struct MeshHandles {
        std::vector<RenderHandle> mesh_handles;
    };
} // namespace enishi::types