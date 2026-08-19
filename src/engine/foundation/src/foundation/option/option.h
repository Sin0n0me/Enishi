#pragma once
#include <optional>
#include <string>
#include <vector>

namespace enishi::foundation {
    template <typename T>
    using OptionExpected = std::conditional_t<std::is_reference_v<T>,
        std::reference_wrapper<std::remove_reference_t<T>>,
        T>;

    namespace details {
        template <typename T> class OptionBase : public std::optional<T> {
          public:
            using std::optional<T>::optional;

            [[nodiscard]] constexpr bool is_some(void) const {
                return this->has_value();
            }

            [[nodiscard]] constexpr bool is_none(void) const {
                return !this->has_value();
            }
        };
    } // namespace details

    template <typename T> class Option : public details::OptionBase<T> {
      public:
        using details::OptionBase<T>::OptionBase;

        [[nodiscard]] const T& unwrap(void) const {
            return this->value();
        }

        [[nodiscard]] T& unwrap_mut(void) {
            return this->value();
        }

        /*
        [[nodiscard]] const T&& unwrap(void) const {
            return this->value();
        }

        [[nodiscard]] T&& unwrap_mut(void) {
            return this->value();
        }
        */
    };

    template <typename T> class Option<T&> : public details::OptionBase<std::reference_wrapper<T>> {
      public:
        using details::OptionBase<std::reference_wrapper<T>>::OptionBase;

        [[nodiscard]] const T& unwrap(void) const {
            return this->value().get();
        }

        [[nodiscard]] T& unwrap_mut(void) {
            return this->value().get();
        }

        /*
        [[nodiscard]] const T&& unwrap(void) const {
            return this->value().get();
        }

        [[nodiscard]] T&& unwrap_mut(void) {
            return this->value().get();
        }
        */
    };
} // namespace enishi::foundation