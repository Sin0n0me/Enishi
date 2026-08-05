#pragma once
#include "../../../handle/handle_type.h"
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

    struct Light {
        glm::vec3 color;
        glm::vec3 direction;
    };

    struct Edge {
        glm::vec3 color;
        float width;
    };

    using MaterialVariant = std::variant<Ambient, Specular, Diffuse, Light, Edge>;

    struct Material {
        std::vector<MaterialVariant> variants; // Vectorに渡した順序に統合されたバイト列になる
        std::vector<std::filesystem::path> texture_paths;
        std::uint32_t indecies;
    };
} // namespace enishi::types