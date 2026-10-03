#pragma once
#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace enishi::foundation {
    namespace details {
        template <typename T>
        using OptionValue = std::conditional_t<std::is_reference_v<T>,
            std::reference_wrapper<std::remove_reference_t<T>>,
            T>;

        template <typename Derived, typename T> class OptionBase {
          private:
            using Option = std::optional<OptionValue<T>>;
            using ConstRefType =
                std::add_lvalue_reference_t<std::add_const_t<std::remove_reference_t<T>>>;

          protected:
            Option option;

          public:
            OptionBase(void) noexcept = default;
            OptionBase(const OptionBase<Derived, T>&) noexcept = default;
            OptionBase(OptionBase<Derived, T>&&) noexcept = default;
            OptionBase(Option&& option) noexcept
                : option(std::move(option)) {
            }
            OptionBase(const Option& option) noexcept
                : option(option) {
            }
            OptionBase(OptionValue<T>&& option) noexcept
                : option(std::move(option)) {
            }
            OptionBase(std::nullopt_t&& nullopt) noexcept
                : option(std::move(nullopt)) {
            }
            OptionBase(const T& value) noexcept
                : option(value) {
            }

          public:
            [[nodiscard]] constexpr bool is_some(void) const {
                return this->option.has_value();
            }

            [[nodiscard]] constexpr bool is_none(void) const {
                return !this->option.has_value();
            }

            [[nodiscard]] ConstRefType unwrap(void) const {
                if constexpr (std::is_reference_v<T>) {
                    return static_cast<ConstRefType>(this->option.value().get());
                } else {
                    return static_cast<const T&>(this->option.value());
                }
            }

            [[nodiscard]] T& unwrap_mut(void) & {
                if constexpr (std::is_reference_v<T>) {
                    return this->option.value().get();
                } else {
                    return this->option.value();
                }
            }
            [[nodiscard]] T&& unwrap_mut(void) && {
                if constexpr (std::is_reference_v<T>) {
                    return this->option.value().get();
                } else {
                    return std::move(this->option).value();
                }
            }

            template <typename U> [[nodiscard]] T unwrap_or(U&& value) const& {
                if constexpr (std::is_reference_v<T>) {
                    return this->option.value_or(std::forward<U>(value)).get();
                } else {
                    return this->option.value_or(std::forward<U>(value));
                }
            }

            template <typename U> [[nodiscard]] T unwrap_or(U&& value) && {
                if constexpr (std::is_reference_v<T>) {
                    return this->option.value_or(std::forward<U>(value)).get();
                } else {
                    return this->option.value_or(std::forward<U>(value));
                }
            }

            [[nodiscard]] T unwrap_or_default(void) const& {
                if constexpr (std::is_reference_v<T>) {
                    return this->option.value_or(T{}).get();
                } else {
                    return this->option.value_or(T{});
                }
            }

            [[nodiscard]] T unwrap_or_default(void) && {
                if constexpr (std::is_reference_v<T>) {
                    return this->option.value_or(T{}).get();
                } else {
                    return this->option.value_or(T{});
                }
            }
        };
    } // namespace details

    template <typename T> class Option : public details::OptionBase<Option<T>, T> {
      public:
        using details::OptionBase<Option<T>, T>::OptionBase;

        Option(void) noexcept = default;
        Option(Option<T>&&) noexcept = default;
        Option(const Option<T>&) noexcept = default;

        Option(details::OptionValue<T>&& value) noexcept
            : details::OptionBase<Option<T>, T>(std::move(value)) {
        }
        Option(std::nullopt_t&& nullopt) noexcept
            : details::OptionBase<Option<T>, T>(std::move(nullopt)) {
        }

        Option& operator=(Option<T>&& value) {
            this->option = value.option;
            return *this;
        }
        Option& operator=(T&& value) {
            this->option = value;
            return *this;
        }
        Option& operator=(std::nullopt_t&& nullopt) {
            this->option = nullopt;
            return *this;
        }

        template <typename F>
        [[nodiscard]]
        constexpr auto and_then(F&& f) && {
            using U = std::invoke_result_t<F, T&>;
            if (this->is_some()) {
                return std::invoke(std::forward<F>(f), std::move(this->unwrap_mut()));
            } else {
                return std::remove_cvref_t<U>();
            }
        }

        template <typename F>
        [[nodiscard]]
        constexpr auto and_then(F&& f) const& {
            using U = std::invoke_result_t<F, T&>;
            if (this->is_some()) {
                return std::invoke(std::forward<F>(f), this->unwrap());
            } else {
                return std::remove_cvref_t<U>();
            }
        }

        template <typename F>
        [[nodiscard]]
        constexpr auto map(F&& f) && {
            using U = std::invoke_result_t<F, T>;
            if (this->is_some()) {
                return Option<U>{std::invoke(std::forward<F>(f), std::move(this->unwrap_mut()))};
            } else {
                return Option<U>{};
            }
        }

        template <typename F>
        [[nodiscard]]
        constexpr auto map(F&& f) const& {
            using U = std::invoke_result_t<F, const T&>;
            if (this->is_some()) {
                return Option<U>{std::invoke(std::forward<F>(f), std::move(this->unwrap()))};
            } else {
                return Option<U>{};
            }
        }
    };
} // namespace enishi::foundation