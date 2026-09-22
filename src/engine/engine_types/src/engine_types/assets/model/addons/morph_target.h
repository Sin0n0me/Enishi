#pragma once
#include <cstdint>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <string>
#include <variant>
#include <vector>

namespace enishi::types {
    struct VertexMorphOffset {
        std::uint32_t vertex;
        glm::vec3 translation;
    };

    struct UVMorphOffset {
        std::uint32_t vertex;
        std::uint32_t channel; // Zero is the primary UV channel.
        glm::vec4 offset;
    };

    struct BoneMorphOffset {
        std::uint32_t bone;
        glm::vec3 translation;
        glm::quat rotation;
    };

    enum class MorphOperation { Multiply, Add };

    struct MaterialPropertyOffset {
        std::string property;
        glm::vec4 value;
    };

    struct MaterialMorphOffset {
        std::uint32_t material; // UINT32_MAX targets every material.
        MorphOperation operation;
        std::vector<MaterialPropertyOffset> properties;
    };

    struct MorphWeightOffset {
        std::uint32_t morph;
        float weight;
    };

    struct ImpulseMorphOffset {
        std::uint32_t rigid_body;
        bool local_space;
        glm::vec3 velocity;
        glm::vec3 torque;
        bool reset_velocity{};
    };

    using MorphOffset = std::variant<VertexMorphOffset,
        UVMorphOffset,
        BoneMorphOffset,
        MaterialMorphOffset,
        MorphWeightOffset,
        ImpulseMorphOffset>;

    enum class MorphWeightMode { Continuous, DiscreteSelection };

    struct MorphTarget {
        std::string name;
        MorphWeightMode weight_mode{MorphWeightMode::Continuous};
        std::vector<MorphOffset> offsets;
    };

    struct AddonMorphTargets {
        std::vector<MorphTarget> targets;
    };
} // namespace enishi::types
