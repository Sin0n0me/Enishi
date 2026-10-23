#include "bone_name_config.h"
#include <foundation/text/config_line_reader.h>
#include <fstream>
#include <iomanip>
#include <set>
#include <sstream>
#include <utility>

namespace enishi::core {
    foundation::Result<animation::BoneNameDictionary, BoneNameConfigError> parse_bone_name_config(
        std::istream& stream) {
        animation::BoneNameDictionary dictionary;
        foundation::ConfigLineReader reader(stream);
        std::istringstream fields;
        std::set<animation::BoneRole> seen;
        while (reader.next(fields)) {
            std::string key;
            fields >> key;
            const auto role = animation::BoneNameDictionary::role_from_key(key);
            if (!role.has_value() || !seen.insert(*role).second) {
                return foundation::Error(BoneNameConfigError::InvalidSetting,
                    "Unknown or duplicate bone role at line " +
                        std::to_string(reader.line_number()));
            }
            std::vector<std::string> names;
            while (!fields.eof()) {
                fields >> std::ws;
                if (fields.eof() || fields.peek() == '#') {
                    break;
                }
                std::string name;
                if (fields.peek() != '"' || !(fields >> std::quoted(name)) || name.empty()) {
                    return foundation::Error(BoneNameConfigError::InvalidSetting,
                        "Expected a quoted nonempty bone name at line " +
                            std::to_string(reader.line_number()));
                }
                names.push_back(std::move(name));
            }
            if (!dictionary.set_aliases(*role, std::move(names))) {
                return foundation::Error(BoneNameConfigError::InvalidSetting,
                    "Missing bone names at line " + std::to_string(reader.line_number()));
            }
        }
        if (reader.read_failed()) {
            return foundation::Error(
                BoneNameConfigError::ReadFailed, "Cannot read bone name settings");
        }
        return dictionary;
    }
    foundation::Result<animation::BoneNameDictionary, BoneNameConfigError> load_bone_name_config(
        const std::filesystem::path& path) {
        std::ifstream stream(path);
        if (!stream.is_open()) {
            return foundation::Error(BoneNameConfigError::ReadFailed,
                "Cannot open bone name configuration: " + path.string());
        }
        return parse_bone_name_config(stream);
    }
} // namespace enishi::core
