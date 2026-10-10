#include "model_event_motion_config.h"
#include <fstream>
#include <iomanip>
#include <sstream>

namespace enishi::core {
    foundation::Result<std::vector<ModelEventMotionRule>, MotionConfigError>
    load_model_event_motions(const std::filesystem::path& path) {
        std::ifstream stream(path);
        if (!stream.is_open()) {
            return foundation::Error(MotionConfigError::ReadFailed,
                "Cannot open model event motion configuration: " + path.string());
        }
        std::vector<ModelEventMotionRule> rules;
        std::string line;
        std::size_t line_number = 0;
        while (std::getline(stream, line)) {
            ++line_number;
            constexpr std::string_view UTF8_BOM = "\xEF\xBB\xBF";
            if (line_number == 1 && line.starts_with(UTF8_BOM)) {
                line.erase(0, UTF8_BOM.size());
            }
            std::istringstream fields(line);
            fields >> std::ws;
            if (fields.eof() || fields.peek() == '#') {
                continue;
            }
            ModelEventMotionRule rule;
            std::string motion;
            std::string mode;
            if (!(fields >> std::quoted(rule.model) >> std::quoted(rule.event) >>
                    std::quoted(rule.bone) >> std::quoted(motion) >> mode) ||
                rule.model.empty() || rule.event.empty() || rule.bone.empty() || motion.empty() ||
                (mode != "off" && mode != "once" && mode != "loop")) {
                return foundation::Error(MotionConfigError::InvalidRule,
                    "Invalid motion rule at line " + std::to_string(line_number));
            }
            fields >> std::ws;
            if (!fields.eof() && fields.peek() != '#') {
                return foundation::Error(MotionConfigError::InvalidRule,
                    "Unexpected motion rule field at line " + std::to_string(line_number));
            }
            const auto relative =
                std::filesystem::path(std::u8string(motion.begin(), motion.end()));
            rule.motion = (path.parent_path() / relative).lexically_normal();
            rule.enabled = mode != "off";
            rule.looping = mode == "loop";
            rules.push_back(std::move(rule));
        }
        if (stream.bad()) {
            return foundation::Error(MotionConfigError::ReadFailed,
                "Cannot read model event motion configuration: " + path.string());
        }
        return rules;
    }
} // namespace enishi::core
