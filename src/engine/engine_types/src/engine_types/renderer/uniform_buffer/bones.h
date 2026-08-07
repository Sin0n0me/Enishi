#pragma once
#include <glm/glm.hpp>

namespace enishi::types {
    template <int SIZE = 512> struct alignas(16) UniformBoneMatrices {
        static constexpr char UNIFORM_NAME[] = "Bones"; // シェーダ側の名前と一致させる必要がある
        static constexpr int CAPACITY = SIZE;
        glm::mat4 bone_matrices[CAPACITY];
    };

    using LightModelBones = UniformBoneMatrices<256>;
    using MediumModelBones = UniformBoneMatrices<512>;
    using HeavyModelBones = UniformBoneMatrices<1024>;
} // namespace enishi::types