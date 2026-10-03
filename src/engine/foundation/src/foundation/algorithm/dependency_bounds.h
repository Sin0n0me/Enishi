#pragma once
#include <cstdint>
#include <vector>

namespace enishi::foundation {
    struct DependencyNode {
        std::size_t id;

        constexpr DependencyNode(void)
            : id(0xFFFF'FFFF) {
        }
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

    struct DependencyDescription {
        DependencyNode node;
        DependencyBounds bounds;
    };
} // namespace enishi::foundation

// ハッシュマップなどのキーとして使用できるようにする
namespace std {
    template <> struct hash<enishi::foundation::DependencyNode> {
        std::size_t operator()(const enishi::foundation::DependencyNode& h) const noexcept {
            return std::hash<decltype(h.id)>{}(h.id);
        }
    };
} // namespace std
