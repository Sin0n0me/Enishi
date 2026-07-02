#pragma once
#include "render_data.h"
#include <cstdint>
#include <glm/glm.hpp>
#include <vector>

namespace enishi::types {
    struct MeshData {
        RenderData vertices;
        RenderData indices;
    };
} // namespace enishi::types
