#include "cursor_pose_config.h"
#include <cmath>
#include <fstream>
#include <set>
#include <sstream>
#include <string>
#include <string_view>

namespace enishi::core {
    foundation::Result<CursorPoseConfig, CursorConfigError> parse_cursor_pose_config(
        std::istream& stream) {
        CursorPoseConfig config;
        std::set<std::string> seen;
        std::string line;
        std::size_t line_number = 0;
        constexpr float MAX_ANGLE = 90.0f;
        constexpr float MAX_SPEED = 720.0f;
        constexpr std::string_view UTF8_BOM = "\xEF\xBB\xBF";
        while (std::getline(stream, line)) {
            ++line_number;
            if (line_number == 1 && line.starts_with(UTF8_BOM)) {
                line.erase(0, UTF8_BOM.size());
            }
            std::istringstream fields(line);
            fields >> std::ws;
            if (fields.eof() || fields.peek() == '#') {
                continue;
            }
            std::string key;
            float value = 0;
            fields >> key;
            float* destination = nullptr;
            if (key == "neck_yaw") {
                destination = &config.neck_yaw;
            } else if (key == "neck_pitch") {
                destination = &config.neck_pitch;
            } else if (key == "head_yaw") {
                destination = &config.head_yaw;
            } else if (key == "head_pitch") {
                destination = &config.head_pitch;
            } else if (key == "arm_speed") {
                destination = &config.arm_speed;
            }
            const float maximum = key == "arm_speed" ? MAX_SPEED : MAX_ANGLE;
            if (destination == nullptr || !seen.insert(key).second || !(fields >> value) ||
                !std::isfinite(value) || value < 0 || value > maximum ||
                (key == "arm_speed" && !(value > 0))) {
                return foundation::Error(CursorConfigError::InvalidSetting,
                    "Invalid cursor setting at line " + std::to_string(line_number));
            }
            fields >> std::ws;
            if (!fields.eof() && fields.peek() != '#') {
                return foundation::Error(CursorConfigError::InvalidSetting,
                    "Unexpected cursor setting field at line " + std::to_string(line_number));
            }
            *destination = value;
        }
        if (stream.bad() || (stream.fail() && !stream.eof())) {
            return foundation::Error(CursorConfigError::ReadFailed, "Cannot read cursor settings");
        }
        return config;
    }

    foundation::Result<CursorPoseConfig, CursorConfigError> load_cursor_pose_config(
        const std::filesystem::path& path) {
        std::ifstream stream(path);
        if (!stream.is_open()) {
            return foundation::Error(CursorConfigError::ReadFailed,
                "Cannot open cursor configuration: " + path.string());
        }
        return parse_cursor_pose_config(stream);
    }
} // namespace enishi::core
