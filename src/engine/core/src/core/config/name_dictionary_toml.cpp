#include "name_dictionary_toml.h"
#define TOML_EXCEPTIONS 0
#include <array>
#include <toml++/toml.hpp>
#include <utility>

namespace enishi::core {
    namespace {
        constexpr std::int64_t supported_version = 1;
        constexpr std::size_t read_buffer_size = 4096;

        foundation::Error<NameDictionaryConfigError> invalid(
            std::string message, const toml::source_region& source) {
            return foundation::Error(NameDictionaryConfigError::InvalidSetting,
                std::move(message) + " at line " + std::to_string(source.begin.line) + ", column " +
                    std::to_string(source.begin.column));
        }

        foundation::Result<NameDictionaryEntry, NameDictionaryConfigError> parse_entry(
            const toml::key& key, const toml::node& node) {
            const auto* table = node.as_table();
            if (table == nullptr) {
                return invalid("Expected a role table", node.source());
            }
            for (const auto& [field, value] : *table) {
                if (field.str() != "aliases") {
                    return invalid(
                        "Unknown role field: " + std::string(field.str()), field.source());
                }
            }
            const auto* aliases = (*table)["aliases"].as_array();
            if (aliases == nullptr || aliases->empty()) {
                return invalid("Expected a nonempty aliases array", node.source());
            }
            NameDictionaryEntry entry{std::string(key.str()), {}, key.source().begin.line};
            for (const auto& alias : *aliases) {
                const auto* name = alias.as_string();
                if (name == nullptr || name->get().empty()) {
                    return invalid("Expected a nonempty alias string", alias.source());
                }
                entry.aliases.push_back(name->get());
            }
            return entry;
        }

        foundation::Result<std::vector<NameDictionaryEntry>, NameDictionaryConfigError> parse_table(
            const toml::table& table) {
            for (const auto& [key, node] : table) {
                if (key.str() != "version" && key.str() != "roles") {
                    return invalid(
                        "Unknown dictionary field: " + std::string(key.str()), key.source());
                }
            }
            const auto* version = table["version"].as_integer();
            if (version == nullptr || version->get() != supported_version) {
                return invalid("Expected dictionary version = 1", table.source());
            }
            std::vector<NameDictionaryEntry> entries;
            const auto* roles_node = table.get("roles");
            if (roles_node == nullptr) {
                return entries;
            }
            const auto* roles = roles_node->as_table();
            if (roles == nullptr) {
                return invalid("Expected a roles table", roles_node->source());
            }
            for (const auto& [key, node] : *roles) {
                auto entry = parse_entry(key, node);
                if (entry.is_err()) {
                    return std::move(entry).take_err();
                }
                entries.push_back(std::move(entry).unwrap_mut());
            }
            return entries;
        }
    } // namespace

    foundation::Result<std::vector<NameDictionaryEntry>, NameDictionaryConfigError>
    parse_name_dictionary_toml(std::istream& stream) {
        std::string source;
        std::array<char, read_buffer_size> buffer;
        while (stream.good()) {
            stream.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
            source.append(buffer.data(), static_cast<std::size_t>(stream.gcount()));
        }
        if (stream.bad() || (stream.fail() && !stream.eof())) {
            return foundation::Error(
                NameDictionaryConfigError::ReadFailed, "Cannot read name dictionary configuration");
        }
        auto parsed = toml::parse(source);
        if (parsed.failed()) {
            return invalid(std::string(parsed.error().description()), parsed.error().source());
        }
        return parse_table(parsed.table());
    }
} // namespace enishi::core
