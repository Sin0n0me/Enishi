#pragma once
#include "../errors/errors.h"
#include "../str/str.h"
#include <expected>
#include <vector>

namespace enishi::foundation {
    namespace details {
        template <typename T, typename E> using WrappedExpected = std::expected<T, Error<E>>;
    } // namespace details

    template <typename T, typename E>
    using Expected = std::conditional_t<std::is_reference_v<T>,
        details::WrappedExpected<std::reference_wrapper<std::remove_reference_t<T>>, E>,
        details::WrappedExpected<T, E>>;
    template <typename E> using Unexpected = std::unexpected<Error<E>>;

    namespace details {
        template <typename Derived, typename T, typename E> class ResultBase {
          private:
            using ConstRefType =
                std::add_lvalue_reference_t<std::add_const_t<std::remove_reference_t<T>>>;

          protected:
            Expected<T, E> expected;

          public:
            ResultBase(ResultBase<Derived, T, E>&&) noexcept = default;
            explicit ResultBase(Expected<T, E>&& expected)
                : expected(std::move(expected)) {
            }
            explicit ResultBase(Unexpected<E>&& unexpected)
                : expected(std::move(unexpected)) {
            }

          public:
            ResultBase& operator=(ResultBase&&) noexcept = default;

          public:
            [[nodiscard]] const T& unwrap(void) const {
                if constexpr (std::is_reference_v<T>) {
                    return static_cast<ConstRefType>(this->expected.value().get());
                } else {
                    return static_cast<const T&>(this->expected.value());
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

            template <typename U> [[nodiscard]] T unwrap_or(U&& value) const& {
                if constexpr (std::is_reference_v<T>) {
                    return static_cast<ConstRefType>(
                        this->expected.value_or(std::forward<U>(value)).get());
                } else {
                    return this->expected.value_or(std::forward<U>(value));
                }
            }

            template <typename U> [[nodiscard]] T unwrap_or(U&& value) && {
                if constexpr (std::is_reference_v<T>) {
                    return static_cast<ConstRefType>(
                        this->expected.value_or(std::forward<U>(value)).get());
                } else {
                    return this->expected.value_or(std::forward<U>(value));
                }
            }

            [[nodiscard]] T unwrap_or_default(void) const& {
                if constexpr (std::is_reference_v<T>) {
                    return this->expected.value_or(T{}).get();
                } else {
                    return this->expected.value_or(T{});
                }
            }

            [[nodiscard]] T unwrap_or_default(void) && {
                if constexpr (std::is_reference_v<T>) {
                    return this->expected.value_or(T{}).get();
                } else {
                    return this->expected.value_or(T{});
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

            [[nodiscard]] Error<E> take_err(void) && {
                return std::move(this->expected.error());
            }

            [[nodiscard]] Derived& add_message(UTF8 message) & {
                if (is_err()) {
                    expected.error().add_message(std::move(message));
                }
                return static_cast<Derived&>(*this);
            }

            [[nodiscard]] Derived add_message(UTF8&& message) && {
                if (this->is_err()) {
                    this->expected.error().add_message(std::move(message));
                }

                return Derived(std::move(static_cast<Derived&>(*this)));
            }

            [[nodiscard]] constexpr bool is_ok(void) const noexcept {
                return this->expected.has_value();
            }

            [[nodiscard]] constexpr bool is_err(void) const noexcept {
                return !this->expected.has_value();
            }

            template <typename U> [[nodiscard]] constexpr Error<U> propagation(U&& e) const {
                return this->expected.error().propagation(std::forward<U>(e));
            }

            template <typename F>
            [[nodiscard]]
            constexpr auto map(F&& f) && {
                using U = std::invoke_result_t<F, T>;
                if (this->is_err()) {
                    return ResultBase<Derived, U, E>{std::move(this->expected.error())};
                }
                return ResultBase<Derived, U, E>{
                    std::invoke(std::forward<F>(f), std::move(this->expected.value()))};
            }

            template <typename F>
            [[nodiscard]]
            constexpr auto map(F&& f) const& {
                using U = std::invoke_result_t<F, const T&>;
                if (this->is_err()) {
                    return ResultBase<Derived, U, E>{std::move(this->expected.error())};
                }
                return ResultBase<Derived, U, E>{
                    std::invoke(std::forward<F>(f), std::move(this->expected.value()))};
            }
        };
    } // namespace details

    template <typename T, typename E>
    class Result : public details::ResultBase<Result<T, E>, T, E> {
      private:
        template <typename U, typename F> friend class Result;

      public:
        using details::ResultBase<Result<T, E>, T, E>::ResultBase;

      public:
        Result(Error<E>&& err)
            : details::ResultBase<Result<T, E>, T, E>(Unexpected<E>{std::move(err)}) {
        }

        template <class U>
            requires std::constructible_from<std::remove_reference_t<T>, U&&>
        Result(U&& value)
            : details::ResultBase<Result<T, E>, T, E>(Expected<T, E>{std::forward<U>(value)}) {
        }

        // UからTへ変換可能な場合
        template <typename U>
            requires std::constructible_from<T, U>
        Result(Result<U, E>&& other)
            : details::ResultBase<Result<T, E>, T, E>(
                  other.is_ok() ? Expected<T, E>{T{std::move(other).unwrap_mut()}}
                                : Expected<T, E>{Unexpected<E>{std::move(other).unwrap_err()}}) {
        }

        Result(Result&&) noexcept = default;

        Result& operator=(Result&&) noexcept = default;
    };

    template <typename E>
    class Result<void, E> : public details::ResultBase<Result<void, E>, std::monostate, E> {
      public:
        using details::ResultBase<Result<void, E>, std::monostate, E>::ResultBase;

      public:
        Result(void)
            : details::ResultBase<Result<void, E>, std::monostate, E>(
                  Expected<std::monostate, E>{}) {
        }
        Result(Error<E>&& err)
            : details::ResultBase<Result<void, E>, std::monostate, E>(
                  Unexpected<E>(std::move(err))) {
        }
        Result(Result&&) noexcept = default;

        Result& operator=(Result&&) noexcept = default;
    };

    template <typename E> using VoidResult = Result<void, E>;
} // namespace enishi::foundation
