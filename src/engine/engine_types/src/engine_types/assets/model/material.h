#pragma once
#include "../../handle/handle_type.h"
#include <cstdint>
#include <filesystem>
#include <glm/glm.hpp>
#include <variant>
#include <vector>

namespace enishi::types {
    struct Ambient {
        glm::vec3 color;
    };

    struct Specular {
        glm::vec3 color;
        float shininess; // スペキュラ係数
    };

    struct Diffuse {
        glm::vec4 color;
    };

    struct IndexCount {
        std::uint32_t count;
    };

    struct MaterialTextures {
        std::vector<std::filesystem::path> paths;
    };

    struct MaterialIndexCount {
        std::uint32_t count;
    };

    using MaterialIndex = std::variant<MaterialIndexCount>;

    using MaterialVariant =
        std::variant<std::monostate, Ambient, Specular, Diffuse, MaterialTextures, MaterialIndex>;

    struct Material {
        std::vector<MaterialVariant> variants;
    };
} // namespace enishi::types