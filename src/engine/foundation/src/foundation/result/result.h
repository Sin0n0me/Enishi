#pragma once
#include "../errors/errors.h"
#include "../str/str.h"
#include <expected>
#include <vector>

namespace enishi::foundation {
    template <typename T, typename E> using Expected = std::expected<T, Error<E>>;
    template <typename E> using Unexpected = std::unexpected<Error<E>>;

    template <typename T, typename E> class ResultBase {
      protected:
        Expected<T, E> expected;

      public:
        ResultBase(ResultBase<T, E>&& result) noexcept
            : expected(std::move(result.expected)) {
        }
        ResultBase(Expected<T, E>&& expected)
            : expected(std::move(expected)) {
        }
        ResultBase(Unexpected<E>&& unexpected)
            : expected(std::move(unexpected)) {
        }

        ResultBase& operator=(ResultBase&&) = default;

        [[nodiscard]] const T& unwrap(void) const {
            if constexpr (std::is_reference_v<T>) {
                return this->expected.value().get();
            } else {
                return this->expected.value();
            }
        }

        [[nodiscard]] T& unwrap_mut(void) & {
            if constexpr (std::is_reference_v<T>) {
                return this->expected.value().get();
            } else {
                return this->expected.value();
            }
        }
        [[nodiscard]] T&& unwrap_mut(void) && {
            if constexpr (std::is_reference_v<T>) {
                return std::move(this->expected.value().get());
            } else {
                return std::move(this->expected.value());
            }
        }

        template <typename U> [[nodiscard]] const T& unwrap_or(U&& value) const {
            if constexpr (std::is_reference_v<T>) {
                return this->expected.value_or(std::forward<U>(value)).get();
            } else {
                return this->expected.value_or(std::forward<U>(value));
            }
        }

        [[nodiscard]] const Error<E>& unwrap_err(void) const& {
            return this->expected.error();
        }
        [[nodiscard]] Error<E>& unwrap_err(void) & {
            return this->expected.error();
        }
        [[nodiscard]] Error<E>&& unwrap_err(void) && {
            return std::move(this->expected.error());
        }
    };

    template <typename T, typename E> class ResultMethod : public ResultBase<T, E> {
      public:
        using ResultBase<T, E>::ResultBase;

      public:
        ResultMethod(ResultMethod&& result)
            : ResultBase<T, E>(std::move(result)) {
        }
        ResultMethod(const Error<E>& err)
            : ResultBase<T, E>(Unexpected<E>(err)) {
        }
        ResultMethod(Error<E>&& err)
            : ResultBase<T, E>(Unexpected<E>(std::move(err))) {
        }
        ResultMethod(details::ErrorBase<E>&& err)
            : ResultBase<T, E>(Unexpected<E>(Error<E>(std::move(err)))) {
        }

        ResultMethod& operator=(ResultMethod&&) = default;

        [[nodiscard]] constexpr bool is_ok(void) const {
            return this->expected.has_value();
        }

        [[nodiscard]] constexpr bool is_err(void) const {
            return !this->expected.has_value();
        }

        template <typename U> [[nodiscard]] constexpr Error<U> propagation(U&& e) const {
            return this->expected.error().propagation(std::forward<U>(e));
        }

        template <typename U> [[nodiscard]] constexpr Error<U> propagation(U&& e) {
            return this->expected.error().propagation(std::forward<U>(e));
        }
    };

    template <typename T, typename E> class Result : public ResultMethod<T, E> {
      private:
        template <typename T, typename E> friend class Result;

      public:
        using ResultMethod<T, E>::ResultMethod;

      public:
        Result(void)
            : ResultMethod<T, E>(Expected<T, E>{T{}}) {
        }
        Result(const T& value)
            : ResultMethod<T, E>(Expected<T, E>{value}) {
        }
        Result(T&& value)
            : ResultMethod<T, E>(Expected<T, E>{std::move(value)}) {
        }
        // UからTへ変換可能な場合
        template <typename U>
            requires std::constructible_from<T, U&&> &&
                     (!std::same_as<std::remove_cvref_t<U>, Result>)
        Result(U&& other)
            : ResultMethod<T, E>(Expected<T, E>{std::forward<U>(other)}) {
        }

        Result(const Error<E>& err)
            : ResultMethod<T, E>(err) {
        }

        Result(Result&& result)
            : ResultMethod<T, E>(std::move(result)) {
        }

        Result& operator=(Result&&) = default;
    };

    template <typename T, typename E>
    class Result<T&, E> : public ResultMethod<std::reference_wrapper<T>, E> {
      public:
        using ResultMethod<std::reference_wrapper<T>, E>::ResultMethod;

        Result(T&& value)
            : ResultMethod<std::reference_wrapper<T>, E>(Expected<std::reference_wrapper<T>, E>{
                  std::forward<std::reference_wrapper<T>>(value)}) {
        }
        Result(Result&& result)
            : ResultMethod<std::reference_wrapper<T>, E>(std::move(result)) {
        }

        Result& operator=(Result&&) = default;
    };

    template <typename E> class Result<void, E> : public ResultMethod<std::monostate, E> {
      public:
        using ResultMethod<std::monostate, E>::ResultMethod;

        Result(void)
            : ResultMethod<std::monostate, E>(Expected<std::monostate, E>{}) {
        }
        Result(Result&& result)
            : ResultMethod<std::monostate, E>(std::move(result)) {
        }

        Result& operator=(Result&&) = default;
    };

    template <typename E> using VoidResult = Result<void, E>;
} // namespace enishi::foundation
