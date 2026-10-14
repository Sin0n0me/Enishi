#pragma once
#include <filesystem>
#include <foundation/result/result.h>
#include <istream>

namespace enishi::core {
    struct CursorPoseConfig {
        float neck_yaw = 10.0f;
        float neck_pitch = 6.0f;
        float head_yaw = 25.0f;
        float head_pitch = 15.0f;
        float arm_speed = 90.0f;
    };

    enum class CursorConfigError { ReadFailed, InvalidSetting };
    [[nodiscard]] foundation::Result<CursorPoseConfig, CursorConfigError> parse_cursor_pose_config(
        std::istream& stream);
    [[nodiscard]] foundation::Result<CursorPoseConfig, CursorConfigError> load_cursor_pose_config(
        const std::filesystem::path& path);
} // namespace enishi::core
