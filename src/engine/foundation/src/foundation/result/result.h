#pragma once
#include "../errors/errors.h"
#include "../str/str.h"
#include <expected>
#include <vector>

namespace enishi::foundation {
    namespace {
        template <typename T, typename E> class ResultBase : public std::expected<T, E> {
          public:
            using std::expected<T, E>::expected;

            [[nodiscard]] constexpr bool is_ok(void) const {
                return this->has_value();
            }

            [[nodiscard]] constexpr bool is_err(void) const {
                return !this->has_value();
            }
        };

        template <typename T, typename E>
        class ResultBase<T&, E> : public std::expected<std::reference_wrapper<T>, E> {
          public:
            using std::expected<std::reference_wrapper<T>, E>::expected;

            [[nodiscard]] constexpr bool is_ok(void) const {
                return this->has_value();
            }

            [[nodiscard]] constexpr bool is_err(void) const {
                return !this->has_value();
            }

            [[nodiscard]] T& unwrap(void) {
                return this->value().get();
            }
        };
    } // namespace

    template <typename T, typename E> class Result : public ResultBase<T, Error<E>> {
      public:
        using ResultBase<T, Error<E>>::ResultBase;

        Result(const Error<E>& err)
            : ResultBase<T, Error<E>>(std::unexpected(err)) {
        }

        template <typename U> [[nodiscard]] constexpr Error<U> propagation(const U e) const {
            return this->error().propagation(e);
        }

        template <typename U>
        [[nodiscard]] constexpr Error<U> propagation(const U e, const UTF8& message) const {
            return this->error().propagation(e, message);
        }

        Result& add_message(const UTF8& message) {
            if (this->is_ok()) {
                return *this;
            }
            this->error().add_message(message);

            return *this;
        }
    };

    template <typename E> class Result<void, E> : public ResultBase<void, Error<E>> {
      public:
        using ResultBase<void, Error<E>>::ResultBase;

        Result(const Error<E>& err)
            : ResultBase<void, Error<E>>(std::unexpected(err)) {
        }

        template <typename U> [[nodiscard]] constexpr Error<U> propagation(const U e) const {
            return this->error().propagation(e);
        }

        template <typename U>
        [[nodiscard]] constexpr Error<U> propagation(const U e, const UTF8& message) const {
            return this->error().propagation(e, message);
        }

        Result& add_message(const UTF8& message) {
            if (this->is_ok()) {
                return *this;
            }
            this->error().add_message(message);

            return *this;
        }
    };

    template <typename E> using VoidResult = Result<void, E>;
} // namespace enishi::foundation
