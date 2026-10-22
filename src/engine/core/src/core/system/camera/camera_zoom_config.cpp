#include "camera_zoom_config.h"
#include <foundation/text/config_line_reader.h>
#include <fstream>
#include <sstream>
#include <string>
#include <string_view>

namespace enishi::core {
    foundation::Result<CameraZoomConfig, CameraZoomConfigError> parse_camera_zoom_config(
        std::istream& stream) {
        CameraZoomConfig config;
        bool seen = false;
        foundation::ConfigLineReader reader(stream);
        std::istringstream fields;
        while (reader.next(fields)) {
            const auto number = reader.line_number();
            std::string key;
            std::string value;
            if (!(fields >> key >> value) || key != "invert_wheel" || seen ||
                (value != "on" && value != "off")) {
                return foundation::Error(CameraZoomConfigError::InvalidSetting,
                    "Invalid camera setting at line " + std::to_string(number));
            }
            if (foundation::has_config_trailing_fields(fields)) {
                return foundation::Error(CameraZoomConfigError::InvalidSetting,
                    "Unexpected camera setting field at line " + std::to_string(number));
            }
            seen = true;
            config.invert_wheel = value == "on";
        }
        if (reader.read_failed()) {
            return foundation::Error(
                CameraZoomConfigError::ReadFailed, "Cannot read camera settings");
        }
        return config;
    }

    foundation::Result<CameraZoomConfig, CameraZoomConfigError> load_camera_zoom_config(
        const std::filesystem::path& path) {
        std::ifstream stream(path);
        if (!stream.is_open()) {
            return foundation::Error(CameraZoomConfigError::ReadFailed,
                "Cannot open camera configuration: " + path.string());
        }
        return parse_camera_zoom_config(stream);
    }
} // namespace enishi::core
