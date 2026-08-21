#pragma once
#include "asset_type.h"
#include <cstdint>
#include <engine_types/handle/handle_type.h>
#include <functional>

namespace enishi::assets_system {
    struct AssetHandle {
        types::HandleId id;
        AssetType type;

        bool is_valid(void) const noexcept;
    };
} // namespace enishi::assets_system

// ハッシュマップなどのキーとして使用できるようにする
namespace std {
    template <> struct hash<enishi::assets_system::AssetHandle> {
        std::size_t operator()(const enishi::assets_system::AssetHandle& h) const noexcept {
            const auto h1 = std::hash<decltype(h.id)>{}(h.id);
            const auto h2 = std::hash<decltype(h.type)>{}(h.type);
            return h1 ^ (h2 < 1);
        }
    };
} // namespace std
