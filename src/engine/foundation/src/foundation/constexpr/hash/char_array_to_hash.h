#pragma once
#include <cstdint>

// constexprなchar[]からconstexprなハッシュ値を求める
namespace enishi ::foundation {
    namespace fnv_1a {
        static constexpr std::uint32_t BASE_32BIT = 0x811C9DC5;
        static constexpr std::uint32_t PRIME_32BIT = 0x01000193;
        static constexpr std::uint64_t BASE_64BIT = 0xCBF29CE484222325;
        static constexpr std::uint64_t PRIME_64BIT = 0x00000100000001B3;
    } // namespace fnv_1a

    // ハッシュ計算本体
    constexpr std::uint32_t hash_u32(const char* str) {
        std::uint32_t h = fnv_1a::BASE_32BIT;
        for (int i = 0; str[i] != '\0'; ++i) {
            h ^= static_cast<std::uint32_t>(str[i]);
            h *= fnv_1a::PRIME_32BIT;
        }
        return h;
    }

    constexpr std::uint64_t hash_u64(const char* str) {
        std::uint64_t h = fnv_1a::BASE_64BIT;
        for (int i = 0; str[i] != '\0'; ++i) {
            h ^= static_cast<std::uint64_t>(str[i]);
            h *= fnv_1a::PRIME_64BIT;
        }
        return h;
    }

    constexpr std::size_t hash_size_t(const char* str) {
        if constexpr (sizeof(std::size_t) == sizeof(std::uint64_t)) {
            return static_cast<std::size_t>(hash_u64(str));
        } else {
            return static_cast<std::size_t>(hash_u32(str));
        }
    }
} // namespace enishi::foundation