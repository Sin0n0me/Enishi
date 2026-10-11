#pragma once
#include <memory>
#include <type_traits>

namespace enishi ::foundation {
    namespace details {
        // T を const T に変換するヘルパー
        template <typename T> struct add_const_ptr {
            using type = const T;
        };

        template <typename T> struct add_const_ptr<std::shared_ptr<T>> {
            using type = std::shared_ptr<const T>;
        };

        template <typename T> struct add_const_ptr<std::unique_ptr<T>> {
            using type = std::unique_ptr<const T>;
        };
    } // namespace details

    template <typename T> using AddConst = typename details::add_const_ptr<T>::type;
} // namespace enishi::foundation