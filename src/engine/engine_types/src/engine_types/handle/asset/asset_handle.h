#pragma once
#include <cstdint>
#include <engine_types/assets/asset_kind.h>
#include <engine_types/handle/handle_type.h>
#include <functional>

namespace enishi::types {
    struct AssetHandle {
        types::HandleId id;
        types::AssetKind type;

        bool is_valid(void) const noexcept;

        bool operator==(const AssetHandle&) const noexcept = default;
    };
} // namespace enishi::types

// ハッシュマップなどのキーとして使用できるようにする
namespace std {
    template <> struct hash<enishi::types::AssetHandle> {
        std::size_t operator()(const enishi::types::AssetHandle& h) const noexcept {
            const auto h1 = std::hash<decltype(h.id)>{}(h.id);
            const auto h2 = std::hash<decltype(h.type)>{}(h.type);
            return h1 ^ (h2 << 1);
        }
    };
} // namespace std
