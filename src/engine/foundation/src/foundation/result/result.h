#pragma once
#include "../errors/errors.h"
#include "../str/str.h"
#include <expected>
#include <vector>

namespace enishi::foundation {
    template <typename T, typename E>
    using storage_t = std::conditional_t<std::is_reference_v<T>,
        std::expected<std::reference_wrapper<std::remove_reference_t<T>>, E>,
        std::expected<T, E>>;

    template <typename T, typename E> class ResultBase {
      protected:
        std::expected<T, Error<E>> expected;

      public:
        ResultBase(ResultBase<T, E>&& result) noexcept
            : expected(std::move(result.expected)) {
        }
        ResultBase(std::expected<T, E>&& expected)
            : expected(std::move(expected)) {
        }
        ResultBase(std::unexpected<E>&& unexpected)
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

        template <typename U> [[nodiscard]] T& unwrap_or(U&& value) {
            if constexpr (std::is_reference_v<T>) {
                return this->expected.value_or(std::forward(value)).get();
            } else {
                return this->expected.value_or(std::forward(value));
            }
        }

        template <typename U> [[nodiscard]] const T& unwrap_or(U&& value) const {
            if constexpr (std::is_reference_v<T>) {
                return this->expected.value_or(std::move(value)).get();
            } else {
                return this->expected.value_or(std::move(value));
            }
        }

        [[nodiscard]] const E& unwrap_err(void) const {
            return this->expected.error();
        }
        [[nodiscard]] E&& unwrap_err(void) {
            return std::move(this->expected.error());
        }
    };

    template <typename E> class ResultBase<void, E> {
      protected:
        std::expected<void, Error<E>> expected;

      public:
        ResultBase(ResultBase<void, E>&& result)
            : expected(std::move(result.expected)) {
        }
        ResultBase(std::expected<void, E>&& expected)
            : expected(expected) {
        }
        ResultBase(std::unexpected<E>&& unexpected)
            : expected(std::move(unexpected)) {
        }

        ResultBase& operator=(ResultBase&&) = default;

        void unwrap(void) const {
            this->expected.value();
        }

        void unwrap_mut(void) {
            return this->expected.value();
        }

        [[nodiscard]] const E& unwrap_err(void) const {
            return this->expected.error();
        }
        [[nodiscard]] E&& unwrap_err(void) {
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
            : ResultBase<T, E>(std::unexpected(err)) {
        }
        ResultMethod(Error<E>&& err)
            : ResultBase<T, E>(std::unexpected(std::move(err))) {
        }
        ResultMethod(ErrorBase<E>&& err)
            : ResultBase<T, E>(std::unexpected(Error<E>(std::move(err)))) {
        }

        ResultMethod& operator=(ResultMethod&&) = default;

        [[nodiscard]] constexpr bool is_ok(void) const {
            return this->expected.has_value();
        }

        [[nodiscard]] constexpr bool is_err(void) const {
            return !this->expected.has_value();
        }

        template <typename U> [[nodiscard]] constexpr Error<U> propagation(const U e) const {
            return this->expected.error().propagation(e);
        }
    };

    template <typename T, typename E> class Result : public ResultMethod<T, E> {
      private:
        template <typename T, typename E> friend class Result;

      public:
        using ResultMethod<T, E>::ResultMethod;

      public:
        Result(void)
            : ResultMethod<T, E>(std::expected<T, Error<E>>{T{}}) {
        }
        Result(const T& value)
            : ResultMethod<T, E>(std::expected<T, Error<E>>{value}) {
        }
        Result(T&& value)
            : ResultMethod<T, E>(std::expected<T, Error<E>>{std::move(value)}) {
        }
        Result(Result&& result)
            : ResultMethod<T, E>(std::move(result)) {
        }
        template <typename U>
            requires std::constructible_from<T, const U&>
        Result(const Result<U, E>& other)
            : ResultMethod<T, E>(std::expected<T, Error<E>>(other.expected)) {
        }

        Result& operator=(Result&&) = default;
    };

    template <typename T, typename E> class Result<T&, E> : public ResultMethod<T&, E> {
      public:
        using ResultMethod<T&, E>::ResultMethod;

        Result(T&& value)
            : ResultMethod<T, E>(
                  std::expected<std::reference_wrapper<T>, Error<E>>{std::forward<T>(value)}) {
        }
        Result(Result&& result)
            : ResultMethod<T&, E>(std::move(result)) {
        }

        Result& operator=(Result&&) = default;
    };

    template <typename E> class Result<void, E> : public ResultMethod<std::monostate, E> {
      public:
        using ResultMethod<std::monostate, E>::ResultMethod;

        Result(void)
            : ResultMethod<std::monostate, E>(std::expected<std::monostate, E>{}) {
        }
        Result(Result&& result)
            : ResultMethod<std::monostate, E>(std::move(result)) {
        }

        Result& operator=(Result&&) = default;
    };

    template <typename E> using VoidResult = Result<void, E>;
} // namespace enishi::foundation
