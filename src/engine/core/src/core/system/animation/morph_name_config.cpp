#include "morph_name_config.h"
#include <foundation/text/config_line_reader.h>
#include <fstream>
#include <iomanip>
#include <set>
#include <sstream>
#include <utility>

namespace enishi::core {
    foundation::Result<animation::MorphNameDictionary, MorphNameConfigError>
    parse_morph_name_config(std::istream& stream) {
        animation::MorphNameDictionary dictionary;
        foundation::ConfigLineReader reader(stream);
        std::istringstream fields;
        std::set<animation::MorphRole> seen;
        while (reader.next(fields)) {
            std::string key;
            fields >> key;
            const auto role = animation::MorphNameDictionary::role_from_key(key);
            if (!role.has_value() || !seen.insert(*role).second) {
                return foundation::Error(MorphNameConfigError::InvalidSetting,
                    "Unknown or duplicate morph role at line " +
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
                    return foundation::Error(MorphNameConfigError::InvalidSetting,
                        "Expected a quoted nonempty morph name at line " +
                            std::to_string(reader.line_number()));
                }
                names.push_back(std::move(name));
            }
            if (!dictionary.set_aliases(*role, std::move(names))) {
                return foundation::Error(MorphNameConfigError::InvalidSetting,
                    "Missing morph names at line " + std::to_string(reader.line_number()));
            }
        }
        if (reader.read_failed()) {
            return foundation::Error(
                MorphNameConfigError::ReadFailed, "Cannot read morph name settings");
        }
        return dictionary;
    }
    foundation::Result<animation::MorphNameDictionary, MorphNameConfigError> load_morph_name_config(
        const std::filesystem::path& path) {
        std::ifstream stream(path);
        if (!stream.is_open()) {
            return foundation::Error(MorphNameConfigError::ReadFailed,
                "Cannot open morph name configuration: " + path.string());
        }
        return parse_morph_name_config(stream);
    }
} // namespace enishi::core
