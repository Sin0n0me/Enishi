#pragma once
#include <filesystem>
#include <foundation/result/result.h>
#include <istream>

namespace enishi::core {
    struct CameraZoomConfig {
        bool invert_wheel = false;
    };
    enum class CameraZoomConfigError { ReadFailed, InvalidSetting };
    [[nodiscard]] foundation::Result<CameraZoomConfig, CameraZoomConfigError>
    parse_camera_zoom_config(std::istream& stream);
    [[nodiscard]] foundation::Result<CameraZoomConfig, CameraZoomConfigError>
    load_camera_zoom_config(const std::filesystem::path& path);
} // namespace enishi::core
