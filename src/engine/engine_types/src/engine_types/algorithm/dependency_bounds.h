#pragma once
#include <cstdint>
#include <vector>

namespace enishi::types {
    struct DependencyNode {
        std::size_t id;

        constexpr DependencyNode(const std::size_t id)
            : id(id) {
        }
        constexpr DependencyNode(const DependencyNode&) = default;
        constexpr DependencyNode(DependencyNode&&) = default;
        constexpr DependencyNode& operator=(DependencyNode&&) = default;
        constexpr bool operator==(const DependencyNode&) const noexcept = default;
    };

    struct DependencyBounds {
        std::vector<DependencyNode> precedents; // 自身の前に必要なもの
    };
} // namespace enishi::types

// ハッシュマップなどのキーとして使用できるようにする
namespace std {
    template <> struct hash<enishi::types::DependencyNode> {
        std::size_t operator()(const enishi::types::DependencyNode& h) const noexcept {
            return std::hash<decltype(h.id)>{}(h.id);
        }
    };
} // namespace std
