#pragma once
#include "../str/str.h"
#include <expected>
#include <format>
#include <memory>
#include <ranges>
#include <type_traits>
#include <utility>
#include <vector>

namespace enishi ::foundation {
    namespace details {
        template <typename Derived, typename E> class ErrorBase {
          protected:
            std::vector<UTF8> messages;
            E error;

          public:
            ErrorBase(const E& error)
                : error(std::move(error)) {
            }
            ErrorBase(E&& error, std::vector<UTF8>&& message)
                : error(std::move(error))
                , messages(std::move(message)) {
            }
            ErrorBase(E&& error, UTF8&& message)
                : error(std::move(error))
                , messages({std::move(message)}) {
            }
            ErrorBase(E&& error, const UTF8& message)
                : error(std::move(error))
                , messages({message}) {
            }

            ErrorBase(const ErrorBase&) = default;
            ErrorBase(ErrorBase&&) = default;
            ErrorBase& operator=(ErrorBase&&) = default;
            ErrorBase& operator=(const ErrorBase&) = default;

          public:
            Derived& add_message(const UTF8& message) & {
                this->messages.emplace_back(message);
                return static_cast<Derived&>(*this);
            }
            Derived& add_message(UTF8&& message) & {
                this->messages.emplace_back(std::move(message));
                return static_cast<Derived&>(*this);
            }
            Derived&& add_message(const UTF8& message) && {
                this->messages.emplace_back(message);
                return static_cast<Derived&&>(std::move(*this));
            }
            Derived&& add_message(UTF8&& message) && {
                this->messages.emplace_back(std::move(message));
                return static_cast<Derived&&>(std::move(*this));
            }

            [[nodiscard]] UTF8 get_message(const UTF8& sep) const {
                const auto joined = this->messages | std::views::join_with(sep);
                return std::string{joined.begin(), joined.end()};
            }

            [[nodiscard]] UTF8 get_message(void) const {
                const auto joined = this->messages | std::views::join_with(std::string_view{"\n"});
                return std::string{joined.begin(), joined.end()};
            }
        };

        template <class Derived> class ErrorBase<Derived, void> {};
    } // namespace details

    template <typename E = std::monostate> class Error : public details::ErrorBase<Error<E>, E> {
      private:
        template <typename T> friend class Error;

      public:
        using details::ErrorBase<Error<E>, E>::ErrorBase;

        Error(const E& error)
            : details::ErrorBase<Error<E>, E>(error) {
        }
        Error(E&& error, UTF8&& message)
            : details::ErrorBase<Error<E>, E>(std::move(error), std::move(message)) {
        }
        Error(E&& error, const UTF8& message)
            : details::ErrorBase<Error<E>, E>(std::move(error), message) {
        }
        template <typename U>
        Error(E&& new_error, Error<U>&& pre_error)
            : details::ErrorBase<E>(std::move(new_error), std::move(pre_error.messages)) {
        }
        Error(details::ErrorBase<Error<E>, E>&& error)
            : details::ErrorBase<Error<E>, E>(error) {
        }

        Error(const Error&) = default;
        Error(Error&&) = default;
        Error& operator=(Error&&) = default;
        Error& operator=(const Error&) = default;

      public:
        // 別のエラーオブジェクト型への変換
        template <class U> Error<U> propagation(U&& e) const& {
            Error<U> ret(std::forward<U>(e));
            ret.messages = std::move(this->messages);
            return ret;
        }
        template <class U> Error<U> propagation(U&& e) && {
            Error<U> ret(std::forward<U>(e));
            ret.messages = std::move(this->messages);
            return ret;
        }

        const E& get_error(void) const {
            return this->error;
        }
    };

    template <> class Error<void> : public details::ErrorBase<Error<void>, std::monostate> {
      public:
        using details::ErrorBase<Error<void>, std::monostate>::ErrorBase;

        Error(void)
            : details::ErrorBase<Error<void>, std::monostate>(std::monostate{}) {
        }
        Error(UTF8&& message)
            : details::ErrorBase<Error<void>, std::monostate>(
                  std::monostate{}, std::move(message)) {
        }
        Error(details::ErrorBase<Error<void>, std::monostate>&& error)
            : details::ErrorBase<Error<void>, std::monostate>(std::move(error)) {
        }

        Error(const Error&) = default;
        Error(Error&&) = default;
        Error& operator=(Error&&) = default;
        Error& operator=(const Error&) = default;
    };
} // namespace enishi::foundation
