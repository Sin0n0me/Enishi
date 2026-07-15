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
        };

        template <typename T, typename E>
        class ResultBase<T&, E> : public std::expected<std::reference_wrapper<T>, E> {
          public:
            using std::expected<std::reference_wrapper<T>, E>::expected;

            [[nodiscard]] const T& unwrap(void) const {
                return this->value().get();
            }

            [[nodiscard]] T& unwrap_mut(void) {
                return this->value().get();
            }
        };

        template <typename T> struct IS_ERROR : std::false_type {};
        template <typename E> struct IS_ERROR<Error<E>> : std::true_type {};
        template <typename T>
        concept ErrorClass = IS_ERROR<T>::value;

        template <typename T, typename E>
            requires ErrorClass<E>
        class ResultMethod : public ResultBase<T, E> {
          public:
            using ResultBase<T, E>::ResultBase;

          public:
            ResultMethod(const ResultMethod&) = default;
            ResultMethod(ResultMethod&&) = default;
            ResultMethod& operator=(ResultMethod&&) = default;

            [[nodiscard]] constexpr bool is_ok(void) const {
                return this->has_value();
            }

            [[nodiscard]] constexpr bool is_err(void) const {
                return !this->has_value();
            }

            const ResultMethod& add_message(const UTF8& message) const {
                if (this->is_ok()) {
                    return *this;
                }
                this->error().add_message(message);

                return *this;
            }

            [[nodiscard]] constexpr Error<E> propagation(void) const {
                return this->error();
            }

            [[nodiscard]] constexpr Error<E> propagation(UTF8& message) const {
                this->add_message(message);
                return this->error();
            }

            template <typename U> [[nodiscard]] constexpr Error<U> propagation(const U e) const {
                return this->error().propagation(e);
            }

            template <typename U>
            [[nodiscard]] constexpr Error<U> propagation(const U e, const UTF8& message) const {
                return this->error().propagation(e, message);
            }
        };
    } // namespace

    template <typename T, typename E> class Result : public ResultMethod<T, Error<E>> {
      public:
        using ResultMethod<T, Error<E>>::ResultMethod;

        Result(const Result&) = default;
        Result(Result&) = default;
        Result(Result&&) = default;

        Result(const Error<E>& err)
            : ResultMethod<T, Error<E>>(std::unexpected(err)) {
        }
        Result(Error<E>&& err)
            : ResultMethod<T, Error<E>>(std::unexpected(std::move(err))) {
        }
        Result(ResultMethod<T, Error<E>>& result)
            : ResultMethod<T, Error<E>>(result) {
        }
        Result(ResultMethod<T, Error<E>>&& result)
            : ResultMethod<T, Error<E>>(result) {
        }
        Result(const ResultMethod<T, Error<E>>& result)
            : ResultMethod<T, Error<E>>(result) {
        }

        Result& operator=(Result&&) = default;
    };

    template <typename E> using VoidResult = Result<void, E>;
} // namespace enishi::foundation
