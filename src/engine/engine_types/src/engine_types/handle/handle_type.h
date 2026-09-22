#pragma once
#include <cstdint>
#include <functional>

namespace enishi::types {
    // バックエンド内の配列インデックスなどに使用
    struct HandleId {
      public:
        using HandleType = std::uint32_t;

      private:
        static constexpr HandleType INITIAL_HANDLE_ID = 0;
        static constexpr HandleType INVALID_HANDLE_ID = UINT32_MAX;

      public:
        HandleType handle_id;

      public:
        constexpr explicit HandleId(HandleType&& id) noexcept
            : handle_id(std::move(id)) {
        }
        constexpr HandleId(void) noexcept
            : handle_id(INVALID_HANDLE_ID) {
        }
        constexpr HandleId(HandleId&&) = default;
        constexpr HandleId(const HandleId& handle)
            : handle_id(handle.handle_id) {};

        constexpr bool is_valid(void) const noexcept {
            return this->handle_id != INVALID_HANDLE_ID;
        }

        constexpr HandleId& operator=(const HandleId&) noexcept = default;
        constexpr HandleId& operator=(HandleId&&) noexcept = default;
        constexpr bool operator==(const HandleId&) const noexcept = default;
    };
} // namespace enishi::types

// ハッシュマップなどのキーとして使用できるようにする
namespace std {
    template <> struct hash<enishi::types::HandleId> {
        std::size_t operator()(const enishi::types::HandleId& h) const noexcept {
            return std::hash<decltype(h.handle_id)>{}(h.handle_id);
        }
    };
} // namespace std
