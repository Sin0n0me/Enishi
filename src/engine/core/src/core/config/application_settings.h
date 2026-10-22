#pragma once
#include <core/system/animation/cursor_pose_config.h>
#include <core/system/animation/model_event_motion_config.h>
#include <core/system/camera/camera_zoom_config.h>

namespace enishi::core {
    struct ApplicationSettings {
        std::vector<ModelEventMotionRule> model_events;
        types::ModelClickSettings click;
        animation::CursorPoseConfig cursor_tracking;
        CameraZoomConfig camera;
    };
    enum class SettingsError { ReadFailed, InvalidSetting };
    [[nodiscard]] foundation::Result<ApplicationSettings, SettingsError> parse_application_settings(
        std::istream& stream, const std::filesystem::path& directory);
    [[nodiscard]] foundation::Result<ApplicationSettings, SettingsError> load_application_settings(
        const std::filesystem::path& path);
} // namespace enishi::core
