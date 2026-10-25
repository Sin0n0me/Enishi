#pragma once
#include <component/animation_component.h>
#include <component/model_component.h>

namespace enishi::core {
    struct SpringTailState {
        glm::vec3 previous{};
        glm::vec3 current{};
        float previous_delta{};
        bool initialized{};
    };

    struct SpringBoneState {
        std::vector<std::vector<SpringTailState>> chains;
    };

    void update_spring_bones(const component::ModelComponent& model,
        component::AnimationComponent& animation,
        SpringBoneState& state,
        float delta_seconds);
} // namespace enishi::core
