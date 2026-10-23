#include "bone_name_config.h"
#include <core/config/name_dictionary_toml.h>
#include <fstream>
#include <utility>

namespace enishi::core {
    foundation::Result<animation::BoneNameDictionary, BoneNameConfigError> parse_bone_name_config(
        std::istream& stream) {
        auto parsed = parse_name_dictionary_toml(stream);
        if (parsed.is_err()) {
            auto code = parsed.unwrap_err().get_error() == NameDictionaryConfigError::ReadFailed
                            ? BoneNameConfigError::ReadFailed
                            : BoneNameConfigError::InvalidSetting;
            return foundation::Error(std::move(code), parsed.unwrap_err().get_message());
        }
        animation::BoneNameDictionary dictionary;
        for (auto& entry : parsed.unwrap_mut()) {
            const auto role = animation::BoneNameDictionary::role_from_key(entry.role);
            if (!role.has_value() || !dictionary.set_aliases(*role, std::move(entry.aliases))) {
                return foundation::Error(BoneNameConfigError::InvalidSetting,
                    "Invalid bone role: " + entry.role + " at line " + std::to_string(entry.line));
            }
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
