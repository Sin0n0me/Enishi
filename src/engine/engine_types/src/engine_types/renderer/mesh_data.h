#pragma once
#include "command/draw_type.h"
#include "render_data.h"
#include <cstdint>
#include <glm/glm.hpp>
#include <string>
#include <unordered_map>
#include <vector>

namespace enishi::types {
    struct MeshData {
        OwnedRenderData vertices;
        OwnedRenderData indices;
        std::unordered_map<std::string, OwnedRenderData>
            uniforms; // first: buffer name second: data
        std::vector<DrawArgs> draw_args;
        std::vector<MeshData> children;
    };
} // namespace enishi::types
