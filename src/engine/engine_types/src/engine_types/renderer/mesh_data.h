#pragma once
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
        std::unordered_map<std::string, OwnedRenderData>
            textures; // first: buffer name second: data
        std::unordered_map<std::string, OwnedRenderData>
            samplders; // first: buffer name second: data
        std::vector<MeshData> children;
    };
} // namespace enishi::types
