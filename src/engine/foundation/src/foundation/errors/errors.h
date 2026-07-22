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
        template <typename E> class ErrorBase {
          protected:
            std::vector<UTF8> messages;
            E error;

          public:
            ErrorBase(E&& error)
                : error(std::move(error))
                , messages({}) {
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
            [[nodiscard]] ErrorBase& add_message(const UTF8& message) & {
                this->messages.emplace_back(message);
                return *this;
            }
            [[nodiscard]] ErrorBase& add_message(UTF8&& message) & {
                this->messages.emplace_back(std::move(message));
                return *this;
            }
            [[nodiscard]] ErrorBase&& add_message(const UTF8& message) && {
                this->messages.emplace_back(message);
                return std::move(*this);
            }
            [[nodiscard]] ErrorBase&& add_message(UTF8&& message) && {
                this->messages.emplace_back(std::move(message));
                return std::move(*this);
            }

            UTF8 get_message(const UTF8& sep) const {
                const auto joined = this->messages | std::views::join_with(sep);
                return std::string{joined.begin(), joined.end()};
            }

            UTF8 get_message(void) const {
                const auto joined = this->messages | std::views::join_with(std::string_view{"\n"});
                return std::string{joined.begin(), joined.end()};
            }
        };

        template <> class ErrorBase<void> {};
    } // namespace details

    template <typename E = std::monostate> class Error : public details::ErrorBase<E> {
      private:
        template <typename T> friend class Error;

      public:
        using details::ErrorBase<E>::ErrorBase;

        Error(E&& error)
            : details::ErrorBase<E>(std::move(error)) {
        }
        Error(E&& error, UTF8&& message)
            : details::ErrorBase<E>(std::move(error), std::move(message)) {
        }
        Error(E&& error, const UTF8& message)
            : details::ErrorBase<E>(std::move(error), message) {
        }
        Error(details::ErrorBase<E>&& error)
            : details::ErrorBase<E>(std::move(error)) {
        }
        template <typename U>
        Error(E&& new_error, Error<U>&& pre_error)
            : details::ErrorBase<E>(std::move(new_error), std::move(pre_error.messages)) {
        }

        Error(const Error&) = default;
        Error(Error&&) = default;
        Error& operator=(Error&&) = default;
        Error& operator=(const Error&) = default;

      public:
        // 別のエラーオブジェクト型への変換
        template <typename U> Error<U> propagation(U&& new_error) const {
            Error<U> e{std::move(new_error)};
            e.messages = this->messages;
            return e;
        }

        const E& get_error(void) const {
            return this->error;
        }
    };

    template <typename E> class Error<E&> : public details::ErrorBase<std::reference_wrapper<E>> {
      public:
        using details::ErrorBase<std::reference_wrapper<E>>::details::ErrorBase;

        Error(details::ErrorBase<std::reference_wrapper<E>>&& error)
            : details::ErrorBase<std::reference_wrapper<E>>(std::move(error)) {
        }

        Error(const Error&) = default;
        Error(Error&&) = default;
        Error& operator=(Error&&) = default;
        Error& operator=(const Error&) = default;
    };

    template <> class Error<void> : public details::ErrorBase<std::monostate> {
      public:
        using details::ErrorBase<std::monostate>::ErrorBase;

        Error(void)
            : details::ErrorBase<std::monostate>(std::monostate{}) {
        }
        Error(UTF8&& message)
            : details::ErrorBase<std::monostate>(std::monostate{}, std::move(message)) {
        }
        Error(details::ErrorBase<std::monostate>&& error)
            : details::ErrorBase<std::monostate>(std::move(error)) {
        }

        Error(const Error&) = default;
        Error(Error&&) = default;
        Error& operator=(Error&&) = default;
        Error& operator=(const Error&) = default;
    };
} // namespace enishi::foundation
