#pragma once
#include "render_data.h"
#include <cstdint>
#include <glm/glm.hpp>
#include <vector>

namespace enishi::types {
    struct MeshData {
        OwnedRenderData<std::byte> vertices;
        OwnedRenderData<std::byte> indices;
        std::vector<OwnedRenderData<std::byte>> uniforms;
        std::vector<MeshData> children;
    };
} // namespace enishi::types
