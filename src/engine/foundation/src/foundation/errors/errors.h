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
    template <typename E> struct ErrorObject {
        E error;

        constexpr ErrorObject(const E& error)
            : error(error) {
        }
        constexpr ErrorObject(E&& error)
            : error(error) {
        }
        constexpr ErrorObject(const ErrorObject&) = default;
        constexpr ErrorObject(ErrorObject&&) = default;
        constexpr ErrorObject& operator=(ErrorObject&&) = default;
    };

    template <> struct ErrorObject<void> {
        constexpr ErrorObject(void) = default;
        constexpr ErrorObject(const ErrorObject&) = default;
        constexpr ErrorObject(ErrorObject&&) = default;
        constexpr ErrorObject& operator=(ErrorObject&&) = default;
    };

    namespace {
        template <typename E> class ErrorBase {
          protected:
            // constなオブジェクトでも上位へ渡しつつ余分なコピーを減らすためにshared_ptr
            std::shared_ptr<std::vector<UTF8>> message;
            ErrorObject<E> error;

          public:
            constexpr ErrorBase(ErrorObject<E>&& error)
                : error(std::move(error)) {
            }
            constexpr ErrorBase(
                ErrorObject<E>&& error, const std::shared_ptr<std::vector<UTF8>>& message)
                : error(std::move(error))
                , message(message) {
            }
            constexpr ErrorBase(ErrorObject<E>&& error, UTF8&& message)
                : error(std::move(error)) {
                this->add_message(std::move(message));
            }

            constexpr ErrorBase(ErrorBase&&) = default;
            constexpr ErrorBase& operator=(ErrorBase&&) = default;

          public:
            void add_message(UTF8&& message) const {
                this->message->emplace_back(std::move(message));
            }

            constexpr UTF8 get_message(const UTF8& sep) const {
                const auto joined = *this->message | std::views::join_with(sep);
                return std::string{joined.begin(), joined.end()};
            }

            constexpr UTF8 get_message(void) const {
                const auto joined = *this->message | std::views::join_with(std::string_view{"\n"});
                return std::string{joined.begin(), joined.end()};
            }

            constexpr const ErrorObject<E>& get_error(void) const {
                return this->error;
            }
        };
    } // namespace

    template <typename E = void> class Error : public ErrorBase<E> {
      private:
        template <typename T> friend class Error;

      public:
        constexpr Error(const E& error)
            : ErrorBase<E>(ErrorObject(error)) {
        }
        constexpr Error(const E& error, UTF8&& message)
            : ErrorBase<E>(ErrorObject(error), std::move(message)) {
        }
        constexpr Error(E&& error)
            : ErrorBase<E>(ErrorObject(error)) {
        }
        constexpr Error(E&& error, UTF8&& message)
            : ErrorBase<E>(ErrorObject(error), std::move(message)) {
        }

        template <typename U>
        constexpr Error(const E& new_error, const Error<U>& error)
            : ErrorBase<E>(ErrorObject(new_error), error.message) {
        }

        constexpr Error(Error&&) = default;
        constexpr Error& operator=(Error&&) = default;

      public:
        // 別のエラーオブジェクト型への変換
        template <typename U> Error<U> propagation(U new_error) const {
            return Error<U>(new_error, *this);
        }

        template <typename U> Error<U> propagation(const U new_error, const UTF8& message) const {
            this->add_message(message);
            return Error<U>(new_error, *this);
        }
    };

    template <> class Error<void> : public ErrorBase<void> {
      private:
        template <typename T> friend class Error;

      public:
        constexpr Error(void)
            : ErrorBase<void>(ErrorObject<void>{}) {
        }
        constexpr Error(UTF8&& message)
            : ErrorBase<void>(ErrorObject<void>{}, std::move(message)) {
        }
    };
} // namespace enishi::foundation
