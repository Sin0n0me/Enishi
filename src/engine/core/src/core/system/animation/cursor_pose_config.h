#pragma once
#include <animation/procedural/cursor/cursor_pose_config.h>
#include <filesystem>
#include <foundation/result/result.h>
#include <istream>

namespace enishi::core {
    enum class CursorConfigError { ReadFailed, InvalidSetting };
    [[nodiscard]] foundation::Result<animation::CursorPoseConfig, CursorConfigError> parse_cursor_pose_config(
        std::istream& stream);
    [[nodiscard]] foundation::Result<animation::CursorPoseConfig, CursorConfigError> load_cursor_pose_config(
        const std::filesystem::path& path);
} // namespace enishi::core
