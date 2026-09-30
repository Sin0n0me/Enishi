#pragma once
#include <animation/clip_data/animation.h>
#include <animation/errors/errors.h>
#include <assets_system/animation/vmd/vmd_data.h>
#include <component/model_component.h>
#include <filesystem>
#include <foundation/result/result.h>

namespace enishi::core {
    [[nodiscard]] foundation::Result<animation::AnimationClipData, animation::AnimationError>
    bind_model_motion(const component::ModelComponent& model, const assets_system::VMDData& data);
    [[nodiscard]] foundation::Result<animation::AnimationClipData, animation::AnimationError>
    load_model_motion(const component::ModelComponent& model, const std::filesystem::path& path);
} // namespace enishi::core
