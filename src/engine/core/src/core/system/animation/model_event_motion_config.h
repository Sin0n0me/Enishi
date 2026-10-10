#pragma once
#include <filesystem>
#include <foundation/result/result.h>
#include <string>
#include <vector>

namespace enishi::core {
    enum class MotionConfigError { ReadFailed, InvalidRule };

    struct ModelEventMotionRule {
        std::string model;
        std::string event;
        std::string bone;
        std::filesystem::path motion;
        bool enabled = false;
        bool looping = false;
    };

    [[nodiscard]] foundation::Result<std::vector<ModelEventMotionRule>, MotionConfigError>
    load_model_event_motions(const std::filesystem::path& path);
} // namespace enishi::core
