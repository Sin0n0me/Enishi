#pragma once
#include <component/model_component.h>
#include <ecs/registry.h>

namespace enishi::core {
    enum class ModelPoseError { InvalidBindTransform, RegistrationFailed };
    [[nodiscard]] foundation::Result<void, ModelPoseError> initialize_model_pose(
        ecs::Registry& registry, types::HandleId entity, const component::ModelComponent& model);
} // namespace enishi::core
