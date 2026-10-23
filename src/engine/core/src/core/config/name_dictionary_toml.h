#pragma once
#include <cstdint>
#include <foundation/result/result.h>
#include <istream>
#include <string>
#include <vector>

namespace enishi::core {
    enum class NameDictionaryConfigError { ReadFailed, InvalidSetting };
    struct NameDictionaryEntry {
        std::string role;
        std::vector<std::string> aliases;
        std::uint32_t line;
    };
    [[nodiscard]] foundation::Result<std::vector<NameDictionaryEntry>, NameDictionaryConfigError>
    parse_name_dictionary_toml(std::istream& stream);
} // namespace enishi::core
