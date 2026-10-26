#include "config_line_reader.h"
#include <string>
#include <string_view>

namespace enishi::foundation {
    ConfigLineReader::ConfigLineReader(std::istream& source)
        : source(source) {
    }

    bool ConfigLineReader::next(std::istringstream& fields) {
        constexpr std::string_view UTF8_BOM = "\xEF\xBB\xBF";
        constexpr std::size_t FIRST_LINE = 1;
        std::string line;
        while (static_cast<bool>(std::getline(this->source, line))) {
            ++this->number;
            if (this->number == FIRST_LINE && line.starts_with(UTF8_BOM)) {
                line.erase(0, UTF8_BOM.size());
            }
            fields.clear();
            fields.str(line);
            fields >> std::ws;
            if (fields.eof() || fields.peek() == '#') {
                continue;
            }
            return true;
        }
        return false;
    }

    std::size_t ConfigLineReader::line_number() const noexcept {
        return this->number;
    }

    bool ConfigLineReader::read_failed() const {
        return this->source.bad() || (this->source.fail() && !this->source.eof());
    }

    bool has_config_trailing_fields(std::istringstream& fields) {
        fields >> std::ws;
        return !fields.eof() && fields.peek() != '#';
    }
} // namespace enishi::foundation
