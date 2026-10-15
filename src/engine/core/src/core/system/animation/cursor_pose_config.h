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
        bool experimental_obb_collision = false;
        bool jump_enabled = true;
        float jump_height_ratio = 0.18f;
        float jump_duration = 0.65f;
        float jump_delay = 0.35f;
        float jump_cooldown = 1.5f;
    };

    enum class CursorConfigError { ReadFailed, InvalidSetting };
    [[nodiscard]] foundation::Result<CursorPoseConfig, CursorConfigError> parse_cursor_pose_config(
        std::istream& stream);
    [[nodiscard]] foundation::Result<CursorPoseConfig, CursorConfigError> load_cursor_pose_config(
        const std::filesystem::path& path);
} // namespace enishi::core
