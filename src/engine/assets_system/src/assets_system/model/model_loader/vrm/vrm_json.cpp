#include "vrm_json.h"
#define CGLTF_IMPLEMENTATION
#include <cgltf.h>
#include <charconv>
#include <cmath>
#include <limits>
#include <locale>
#include <sstream>

namespace enishi::assets_system {
    namespace {
        constexpr std::size_t MAX_JSON_DEPTH = 128;

        VRMJson read_value(const char* text,
            const std::vector<jsmntok_t>& tokens,
            std::size_t& cursor,
            std::size_t depth) {
            if (!(cursor < tokens.size()) || !(depth < MAX_JSON_DEPTH)) {
                return {};
            }
            const auto token = tokens[cursor++];
            VRMJson result;
            result.valid = true;
            if (token.type == JSMN_OBJECT) {
                for (int member = 0; member < token.size; ++member) {
                    auto key = read_value(text, tokens, cursor, depth + 1);
                    auto value = read_value(text, tokens, cursor, depth + 1);
                    if (!key.valid || !value.valid || result.members.contains(key.value)) {
                        return {};
                    }
                    result.members.emplace(std::move(key.value), std::move(value));
                }
            } else if (token.type == JSMN_ARRAY) {
                for (int element = 0; element < token.size; ++element) {
                    auto value = read_value(text, tokens, cursor, depth + 1);
                    if (!value.valid) {
                        return {};
                    }
                    result.elements.push_back(std::move(value));
                }
            } else {
                result.value.assign(text + token.start, text + token.end);
                if (token.type == JSMN_STRING) {
                    result.value.resize(cgltf_decode_string(result.value.data()));
                }
            }
            return result;
        }
    } // namespace

    VRMJson VRMJson::parse(const char* text) {
        if (text == nullptr) {
            return {};
        }
        const auto length = std::char_traits<char>::length(text);
        jsmn_parser parser;
        jsmn_init(&parser);
        const auto count = jsmn_parse(&parser, text, length, nullptr, 0);
        if (!(count > 0)) {
            return {};
        }
        std::vector<jsmntok_t> tokens(static_cast<std::size_t>(count));
        jsmn_init(&parser);
        if (jsmn_parse(&parser, text, length, tokens.data(), tokens.size()) != count) {
            return {};
        }
        std::size_t cursor{};
        auto result = read_value(text, tokens, cursor, 0);
        if (cursor != tokens.size()) {
            return {};
        }
        return result;
    }

    const VRMJson& VRMJson::operator[](std::string_view key) const {
        static const VRMJson missing;
        const auto found = this->members.find(std::string(key));
        return found == this->members.end() ? missing : found->second;
    }

    float VRMJson::number(float fallback) const {
        if (!this->valid) {
            return fallback;
        }
        constexpr std::string_view DECIMAL_CHARACTERS = "-+0123456789.eE";
        if (this->value.starts_with('+') ||
            this->value.find_first_not_of(DECIMAL_CHARACTERS) != std::string::npos) {
            return std::numeric_limits<float>::quiet_NaN();
        }
        // Parse through double so representable float subnormals do not raise ERANGE
        // in standard libraries that use strtof for float stream extraction.
        double parsed{};
        // Floating-point from_chars is unavailable on some supported macOS targets.
        // The classic locale keeps JSON's decimal point independent of the user's locale.
        std::istringstream input(this->value);
        input.imbue(std::locale::classic());
        input >> std::noskipws >> parsed;
        if (input.fail() || !input.eof() || !std::isfinite(parsed) ||
            std::abs(parsed) > static_cast<double>(std::numeric_limits<float>::max())) {
            return std::numeric_limits<float>::quiet_NaN();
        }
        const auto result = static_cast<float>(parsed);
        // Streams can silently round an underflow to zero; preserve from_chars' rejection.
        const auto significand =
            std::string_view(this->value).substr(0, this->value.find_first_of("eE"));
        if (result == 0.0f && significand.find_first_of("123456789") != std::string_view::npos) {
            return std::numeric_limits<float>::quiet_NaN();
        }
        return result;
    }

    std::size_t VRMJson::index() const {
        std::size_t result{};
        const auto parsed =
            std::from_chars(this->value.data(), this->value.data() + this->value.size(), result);
        if (!this->valid || parsed.ec != std::errc{} ||
            parsed.ptr != this->value.data() + this->value.size()) {
            return std::numeric_limits<std::size_t>::max();
        }
        return result;
    }

    bool VRMJson::boolean() const {
        return this->value == "true";
    }
} // namespace enishi::assets_system
