#pragma once
#include <array>
#include <engine_types/input/model_click_settings.h>
#include <filesystem>
#include <foundation/result/result.h>
#include <istream>
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
    parse_model_event_motions(std::istream& stream,
        const std::filesystem::path& directory,
        types::ModelClickSettings* settings = nullptr);
    [[nodiscard]] foundation::Result<std::vector<ModelEventMotionRule>, MotionConfigError>
    load_model_event_motions(
        const std::filesystem::path& path, types::ModelClickSettings* settings = nullptr);
} // namespace enishi::core
