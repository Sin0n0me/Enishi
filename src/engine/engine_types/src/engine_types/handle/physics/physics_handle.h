#pragma once
#include "../handle_type.h"
#include <cstdint>
#include <functional>

namespace enishi::types {
    enum class PhysicsHandleType : std::uint32_t {
        Unknown = 0,
        RigidBody,
        Joint,
        Shape,
        MotionState,
    };

    struct PhysicsHandle {
        HandleId id;
        PhysicsHandleType type;

        explicit PhysicsHandle(const HandleId id, const PhysicsHandleType type);
        PhysicsHandle(void);
        PhysicsHandle(PhysicsHandle&&) = default;
        PhysicsHandle(const PhysicsHandle& handle);

        bool is_valid(void) const noexcept;

        constexpr bool operator==(const PhysicsHandle&) const = default;
        constexpr PhysicsHandle& operator=(const PhysicsHandle&) noexcept = default;
        constexpr PhysicsHandle& operator=(PhysicsHandle&&) noexcept = default;
    };
} // namespace enishi::types

// ハッシュマップなどのキーとして使用できるようにする
namespace std {
    template <> struct hash<enishi::types::PhysicsHandle> {
        std::size_t operator()(const enishi::types::PhysicsHandle& p) const noexcept {
            using Type = enishi::types::PhysicsHandle;
            const std::size_t h1 = std::hash<decltype(Type::id)>{}(p.id);
            const std::size_t h2 = std::hash<decltype(Type::type)>{}(p.type);
            return h1 ^ (h2 << 1);
        }
    };
} // namespace std
