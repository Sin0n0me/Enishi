#pragma once
#include "../binding_description.h"
#include <cstdint>
#include <engine_types/renderer/render_data.h>
#include <foundation/str/str.h>
#include <glm/glm.hpp>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace enishi::renderer {
    template <typename T> using MeshResourceMap = std::unordered_map<foundation::UTF8, T>;

    struct MeshMaterial {
        using BindTextureMap = MeshResourceMap<std::shared_ptr<types::TextureData>>;
        DrawBinding draw_binding;
        BindTextureMap textures;
    };

    struct MeshData {
        using UniformMap = MeshResourceMap<types::OwnedRenderData>;

        types::OwnedRenderData vertices;
        types::OwnedRenderData indices;
        UniformMap uniforms; // first: buffer name second: data
        std::vector<MeshMaterial> materials;
    };
} // namespace enishi::renderer
