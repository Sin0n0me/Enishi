#pragma once
#include <cstdint>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <vector>

namespace enishi::component {
    struct AnimationBuffer {
        glm::vec3 position;
        glm::vec3 scale;
        glm::quat rotation;
    };

    using BufferIndex = std::uint32_t;

    // std::unordered_map<MultiplyOrder, BufferIndex> order_map;
    // std::vector<AnimationCommand> commands;
    // std::vector<MultiplyOrder> multiply_order;

    struct AnimationComponent {
        std::vector<AnimationBuffer> animation;
        std::vector<glm::mat4> global;
        float elapsed_time; // 経過時間
    };
} // namespace enishi::component
