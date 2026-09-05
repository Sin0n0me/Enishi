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

    struct Camera {};

    using MaterialVariant = std::variant<Ambient,
        Specular,
        Diffuse,
        Light,
        Edge,
        glm::u32vec1,
        glm::u32vec2,
        glm::u32vec3,
        glm::u32vec4,
        glm::i32vec1,
        glm::i32vec2,
        glm::i32vec3,
        glm::i32vec4,
        glm::f32vec1,
        glm::f32vec2,
        glm::f32vec3,
        glm::f32vec4,
        glm::u32mat3x3,
        glm::u32mat4x4,
        glm::i32mat3x3,
        glm::i32mat4x4,
        glm::f32mat3x3,
        glm::f32mat4x4>;

    struct MaterialTexture {
        std::filesystem::path path;
        std::string texture_target_name;
        std::string sampler_target_name;
    };

    struct Material {
        std::string name;
        std::vector<MaterialVariant> variants; // Vectorに渡した順序に統合されたバイト列になる
        std::vector<MaterialTexture> textures;
        std::uint32_t first_offset;
        std::uint32_t first_instance_offset;
        std::uint32_t count; // 頂点 or インデックス
        std::uint32_t instance_count;
    };
} // namespace enishi::types