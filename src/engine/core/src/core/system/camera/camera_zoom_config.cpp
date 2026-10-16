#include "camera_zoom_config.h"
#include <fstream>
#include <sstream>
#include <string>
#include <string_view>

namespace enishi::core {
    foundation::Result<CameraZoomConfig, CameraZoomConfigError> parse_camera_zoom_config(
        std::istream& stream) {
        CameraZoomConfig config;
        bool seen = false;
        std::string line;
        std::size_t number = 0;
        constexpr std::string_view UTF8_BOM = "\xEF\xBB\xBF";
        while (std::getline(stream, line)) {
            ++number;
            if (number == 1 && line.starts_with(UTF8_BOM)) {
                line.erase(0, UTF8_BOM.size());
            }
            std::istringstream fields(line);
            fields >> std::ws;
            if (fields.eof() || fields.peek() == '#') {
                continue;
            }
            std::string key;
            std::string value;
            if (!(fields >> key >> value) || key != "invert_wheel" || seen ||
                (value != "on" && value != "off")) {
                return foundation::Error(CameraZoomConfigError::InvalidSetting,
                    "Invalid camera setting at line " + std::to_string(number));
            }
            fields >> std::ws;
            if (!fields.eof() && fields.peek() != '#') {
                return foundation::Error(CameraZoomConfigError::InvalidSetting,
                    "Unexpected camera setting field at line " + std::to_string(number));
            }
            seen = true;
            config.invert_wheel = value == "on";
        }
        if (stream.bad() || (stream.fail() && !stream.eof())) {
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
