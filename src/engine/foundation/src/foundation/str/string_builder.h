#pragma once
#include "str.h"
#include <deque>
#include <ranges>
#include <string_view>
#include <type_traits>
#include <utility>

namespace enishi ::foundation {
    class StringBuilder {
      private:
        std::deque<UTF8> strings;

      public:
        void push_front(UTF8&& message);

        void push_back(UTF8&& message);

        void add_indent(UTF8&& indent, const std::uint32_t count);

        UTF8 join(UTF8&& sep) const;
    };
} // namespace enishi::foundation