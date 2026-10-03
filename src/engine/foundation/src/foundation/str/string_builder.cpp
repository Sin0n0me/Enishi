#include "string_builder.h"

namespace enishi ::foundation {
    void StringBuilder::push_front(UTF8&& message) {
        this->strings.emplace_front(std::move(message));
    }

    void StringBuilder ::push_back(UTF8&& message) {
        this->strings.emplace_back(std::move(message));
    }

    void StringBuilder::add_indent(UTF8&& indent, const std::uint32_t count) {
        UTF8 prefix;
        prefix.reserve(indent.size() * count);
        for (std::size_t i = 0; i < count; ++i) {
            prefix += indent;
        }

        for (auto& line : this->strings) {
            line = prefix + line;
        }
    }

    UTF8 StringBuilder ::join(UTF8&& sep) const {
        return this->strings | std::views::join_with(std::string_view{sep}) |
               std::ranges::to<std::string>();
    }
} // namespace enishi::foundation