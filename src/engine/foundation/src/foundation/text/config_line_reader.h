#pragma once
#include <cstddef>
#include <istream>
#include <sstream>

namespace enishi::foundation {
    class ConfigLineReader {
      private:
        std::istream& source;
        std::size_t number = 0;

      public:
        explicit ConfigLineReader(std::istream& source);
        [[nodiscard]] bool next(std::istringstream& fields);
        [[nodiscard]] std::size_t line_number() const noexcept;
        [[nodiscard]] bool read_failed() const;
    };
    [[nodiscard]] bool has_config_trailing_fields(std::istringstream& fields);
} // namespace enishi::foundation
