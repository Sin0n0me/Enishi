#include "morph_name_config.h"
#include <core/config/name_dictionary_toml.h>
#include <fstream>
#include <utility>

namespace enishi::core {
    foundation::Result<animation::MorphNameDictionary, MorphNameConfigError>
    parse_morph_name_config(std::istream& stream) {
        auto parsed = parse_name_dictionary_toml(stream);
        if (parsed.is_err()) {
            auto code = parsed.unwrap_err().get_error() == NameDictionaryConfigError::ReadFailed
                            ? MorphNameConfigError::ReadFailed
                            : MorphNameConfigError::InvalidSetting;
            return foundation::Error(std::move(code), parsed.unwrap_err().get_message());
        }
        animation::MorphNameDictionary dictionary;
        for (auto& entry : parsed.unwrap_mut()) {
            const auto role = animation::MorphNameDictionary::role_from_key(entry.role);
            if (role.is_none() ||
                !dictionary.set_aliases(role.unwrap(), std::move(entry.aliases))) {
                return foundation::Error(MorphNameConfigError::InvalidSetting,
                    "Invalid morph role: " + entry.role + " at line " + std::to_string(entry.line));
            }
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
