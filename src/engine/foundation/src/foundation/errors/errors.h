#pragma once
#include "../str/str.h"
#include <expected>
#include <format>
#include <ranges>
#include <vector>

namespace enishi ::foundation {
    template <typename E> class Error {
      private:
        std::vector<UTF8> message;
        E error;

      private:
        template <typename U> friend class Error;

      public:
        explicit Error(const E error)
            : error(error)
            , message() {
        }
        explicit Error(const E error, const UTF8& message)
            : error(error)
            , message() {
            this->add_message(message);
        }

        void add_message(const UTF8& message) {
            this->message.push_back(message);
        }

        UTF8 get_message(const UTF8& sep) const {
            const auto joined = this->message | std::views::join_with(sep);
            return std::string{joined.begin(), joined.end()};
        }

        UTF8 get_message(void) const {
            const auto joined = this->message | std::views::join_with(std::string_view{"\n"});
            return std::string{joined.begin(), joined.end()};
        }

        template <typename T> Error<T> propagation(const T new_error) const {
            Error<T> next_err = Error<T>(new_error);
            next_err.message = this->message;
            return next_err;
        }

        template <typename T> Error<T> propagation(const T new_error, const UTF8& message) const {
            Error<T> next_err = Error<T>(new_error);
            next_err.message = this->message;
            next_err.message.push_back(message);
            return next_err;
        }
    };
} // namespace enishi::foundation
