#pragma once
#include <array>
#include <filesystem>
#include <foundation/result/result.h>
#include <string>
#include <vector>

namespace enishi::core {
    enum class MotionConfigError { ReadFailed, InvalidRule };
    enum class ModelEventAction { Motion, Impulse, None };

    struct ModelEventMotionRule {
        std::string model;
        std::string event;
        std::string bone;
        std::filesystem::path motion;
        bool enabled = false;
        bool looping = false;
        ModelEventAction action = ModelEventAction::Motion;
        std::array<float, 3> impulse{};
    };

    [[nodiscard]] foundation::Result<std::vector<ModelEventMotionRule>, MotionConfigError>
    load_model_event_motions(const std::filesystem::path& path);
} // namespace enishi::core
