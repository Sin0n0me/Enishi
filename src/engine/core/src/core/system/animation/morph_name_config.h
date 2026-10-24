#pragma once
#include <animation/morph/morph_name_dictionary.h>
#include <filesystem>
#include <foundation/result/result.h>
#include <istream>

namespace enishi::core {
    enum class MorphNameConfigError { ReadFailed, InvalidSetting };
    [[nodiscard]] foundation::Result<animation::MorphNameDictionary, MorphNameConfigError>
    parse_morph_name_config(std::istream& stream);
    [[nodiscard]] foundation::Result<animation::MorphNameDictionary, MorphNameConfigError>
    load_morph_name_config(const std::filesystem::path& path);
} // namespace enishi::core
