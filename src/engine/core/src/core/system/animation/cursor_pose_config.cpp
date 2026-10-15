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
        constexpr float MAX_HEIGHT_RATIO = 0.5f;
        constexpr float MAX_JUMP_SECONDS = 10.0f;
        constexpr float MIN_JUMP_DURATION = 0.2f;
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
            if (!seen.insert(key).second) {
                return foundation::Error(CursorConfigError::InvalidSetting,
                    "Duplicate cursor setting at line " + std::to_string(line_number));
            }
            float* destination = nullptr;
            float maximum = MAX_ANGLE;
            float minimum = 0;
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
                maximum = MAX_SPEED;
            } else if (key == "jump_height_ratio") {
                destination = &config.jump_height_ratio;
                maximum = MAX_HEIGHT_RATIO;
            } else if (key == "jump_duration") {
                destination = &config.jump_duration;
                minimum = MIN_JUMP_DURATION;
                maximum = MAX_JUMP_SECONDS;
            } else if (key == "jump_delay") {
                destination = &config.jump_delay;
                maximum = MAX_JUMP_SECONDS;
            } else if (key == "jump_cooldown") {
                destination = &config.jump_cooldown;
                maximum = MAX_JUMP_SECONDS;
            }
            if (key == "experimental_obb_collision" || key == "jump_enabled") {
                std::string mode;
                if (!(fields >> mode) || (mode != "on" && mode != "off")) {
                    return foundation::Error(CursorConfigError::InvalidSetting,
                        "Invalid cursor toggle at line " + std::to_string(line_number));
                }
                if (key == "jump_enabled") {
                    config.jump_enabled = mode == "on";
                } else {
                    config.experimental_obb_collision = mode == "on";
                }
            } else if (destination == nullptr || !(fields >> value) || !std::isfinite(value) ||
                       value < minimum || value > maximum || (key == "arm_speed" && !(value > 0))) {
                return foundation::Error(CursorConfigError::InvalidSetting,
                    "Invalid cursor setting at line " + std::to_string(line_number));
            }
            fields >> std::ws;
            if (!fields.eof() && fields.peek() != '#') {
                return foundation::Error(CursorConfigError::InvalidSetting,
                    "Unexpected cursor setting field at line " + std::to_string(line_number));
            }
            if (destination != nullptr) {
                *destination = value;
            }
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
