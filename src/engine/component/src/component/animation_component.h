#pragma once
#include <cstdint>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <vector>

namespace enishi::component {
    struct AnimationBuffer {
        glm::vec3 position{};
        glm::vec3 scale{1.0f};
        glm::quat rotation{1.0f, 0.0f, 0.0f, 0.0f};
    };

    using BufferIndex = std::uint32_t;

    // std::unordered_map<MultiplyOrder, BufferIndex> order_map;
    // std::vector<AnimationCommand> commands;
    // std::vector<MultiplyOrder> multiply_order;

    struct AnimationComponent {
        std::vector<AnimationBuffer> animation;
        std::vector<glm::mat4> global;
        float elapsed_time{}; // 経過時間
        std::vector<AnimationBuffer> bind_pose;
    };
} // namespace enishi::component
