#include "model_event_motion_config.h"
#include <cmath>
#include <fstream>
#include <iomanip>
#include <sstream>

namespace enishi::core {
    namespace {
        bool read_push_states(std::istringstream& fields, types::ModelClickSettings& settings) {
            settings.push_states.clear();
            std::string state;
            bool exclusive = false;
            bool any = false;
            while (fields >> state) {
                if (state.starts_with('#')) {
                    break;
                }
                if (exclusive) {
                    return false;
                }
                if (state == "all" || state == "none") {
                    if (any) {
                        return false;
                    }
                    exclusive = true;
                    if (state == "all") {
                        settings.push_states = types::ModelClickSettings{}.push_states;
                    }
                } else if (state == "Kinematic") {
                    settings.push_states.push_back(types::RigidBodyKind::Kinematic);
                } else if (state == "Dynamic") {
                    settings.push_states.push_back(types::RigidBodyKind::Dynamic);
                } else if (state == "DynamicAdjustBone") {
                    settings.push_states.push_back(types::RigidBodyKind::DynamicAdjustBone);
                } else {
                    return false;
                }
                any = true;
            }
            return any;
        }

        bool read_action(std::istringstream& fields,
            const std::filesystem::path& directory,
            ModelEventMotionRule& rule) {
            std::string action;
            if (!(fields >> std::quoted(action)) || action.empty()) {
                return false;
            }
            if (action == "none") {
                rule.action = ModelEventAction::None;
                rule.enabled = true;
                return true;
            }
            std::string mode;
            if (action == "impulse") {
                rule.action = ModelEventAction::Impulse;
                for (auto& value : rule.impulse) {
                    if (!(fields >> value) || !std::isfinite(value)) {
                        return false;
                    }
                }
                if (!(fields >> mode) || (mode != "on" && mode != "off")) {
                    return false;
                }
                rule.enabled = mode == "on";
                return true;
            }
            // Preserve existing rows whose fourth field is a motion path.
            if (action == "motion" && (!(fields >> std::quoted(action)) || action.empty())) {
                return false;
            }
            // Match impulse's on/off spelling; on plays a motion once.
            if (!(fields >> mode) ||
                (mode != "off" && mode != "on" && mode != "once" && mode != "loop")) {
                return false;
            }
            rule.motion =
                (directory / std::filesystem::path(std::u8string(action.begin(), action.end())))
                    .lexically_normal();
            rule.enabled = mode != "off";
            rule.looping = mode == "loop";
            return true;
        }
    } // namespace

    foundation::Result<std::vector<ModelEventMotionRule>, MotionConfigError>
    load_model_event_motions(const std::filesystem::path& path, types::ModelClickSettings* settings) {
        types::ModelClickSettings parsed_settings;
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
            std::string directive;
            fields >> directive;
            if (directive == "push_states") {
                if (!read_push_states(fields, parsed_settings)) {
                    return foundation::Error(MotionConfigError::InvalidRule,
                        "Invalid push_states at line " + std::to_string(line_number));
                }
                continue;
            }
            fields.clear();
            fields.seekg(0);
            ModelEventMotionRule rule;
            if (!(fields >> std::quoted(rule.model) >> std::quoted(rule.event) >>
                    std::quoted(rule.bone)) ||
                rule.model.empty() || rule.event.empty() || rule.bone.empty() ||
                !read_action(fields, path.parent_path(), rule)) {
                return foundation::Error(MotionConfigError::InvalidRule,
                    "Invalid motion rule at line " + std::to_string(line_number));
            }
            fields >> std::ws;
            if (!fields.eof() && fields.peek() != '#') {
                return foundation::Error(MotionConfigError::InvalidRule,
                    "Unexpected motion rule field at line " + std::to_string(line_number));
            }
            rules.push_back(std::move(rule));
        }
        if (stream.bad()) {
            return foundation::Error(MotionConfigError::ReadFailed,
                "Cannot read model event motion configuration: " + path.string());
        }
        if (settings != nullptr) {
            settings->push_states = std::move(parsed_settings.push_states);
        }
        return rules;
    }
} // namespace enishi::core
