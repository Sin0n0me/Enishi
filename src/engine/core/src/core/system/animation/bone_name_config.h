#pragma once
#include <animation/rig/bone_name_dictionary.h>
#include <filesystem>
#include <foundation/result/result.h>
#include <istream>

namespace enishi::core {
    enum class BoneNameConfigError { ReadFailed, InvalidSetting };
    [[nodiscard]] foundation::Result<animation::BoneNameDictionary, BoneNameConfigError>
    parse_bone_name_config(std::istream& stream);
    [[nodiscard]] foundation::Result<animation::BoneNameDictionary, BoneNameConfigError>
    load_bone_name_config(const std::filesystem::path& path);
} // namespace enishi::core
