#pragma once
#include <cstdint>
#include <engine_types/renderer/binding_description.h>
#include <engine_types/renderer/render_data.h>
#include <glm/glm.hpp>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace enishi::types {
    template <typename T> using MeshResourceMap = std::unordered_map<std::string, T>;

    struct MeshMaterial {
        using BindTextureMap = MeshResourceMap<std::shared_ptr<types::TextureData>>;
        types::DrawBinding draw_binding;
        BindTextureMap textures;
    };

    struct MeshData {
        using UniformMap = MeshResourceMap<types::OwnedRenderData>;

        types::OwnedRenderData vertices;
        types::OwnedRenderData indices;
        UniformMap uniforms; // first: buffer name second: data
        std::vector<MeshMaterial> materials;
    };
} // namespace enishi::types
