#pragma once
#include <cstdint>
#include <glm/glm.hpp>

namespace enishi::types {
    template <std::uint32_t SIZE = 512> struct alignas(16) UniformBoneMatrices {
        using MatrixType = glm::mat4;
        static constexpr char UNIFORM_NAME[] = "Bones"; // シェーダ側の名前と一致させる必要がある
        static constexpr std::uint32_t CAPACITY = SIZE;
        MatrixType bone_matrices[CAPACITY];
    };

    using LightModelBones = UniformBoneMatrices<256>;
    using MediumModelBones = UniformBoneMatrices<512>;
    using HeavyModelBones = UniformBoneMatrices<1024>;
} // namespace enishi::types