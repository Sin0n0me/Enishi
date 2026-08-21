#pragma once
#include "../handle_type.h"
#include <cstdint>
#include <functional>

namespace enishi::types {
    // バッファーの種類などはHandleIDから判断する
    // あくまで何かを示すだけ
    enum class RenderHandleType : std::uint32_t {
        Unknown = 0,
        Mesh,
        Texture,
        Shader,
        Buffer,
        View,
        Rasterizer,
        ViewPort,
        Topology,
        VertexLayout,
        Draw,
        ShaderReflection,
    };

    struct RenderHandle {
        HandleId id;
        RenderHandleType type;

        explicit RenderHandle(const HandleId id, const RenderHandleType type);
        RenderHandle(void);
        RenderHandle(RenderHandle&&) = default;
        RenderHandle(const RenderHandle& handle);

        bool is_valid(void) const noexcept;

        constexpr bool operator==(const RenderHandle&) const = default;
        constexpr RenderHandle& operator=(const RenderHandle&) noexcept = default;
        constexpr RenderHandle& operator=(RenderHandle&&) noexcept = default;
    };
} // namespace enishi::types

// ハッシュマップなどのキーとして使用できるようにする
namespace std {
    template <> struct hash<enishi::types::RenderHandle> {
        std::size_t operator()(const enishi::types::RenderHandle& p) const noexcept {
            using Type = enishi::types::RenderHandle;
            const std::size_t h1 = std::hash<decltype(Type::id)>{}(p.id);
            const std::size_t h2 = std::hash<decltype(Type::type)>{}(p.type);
            return h1 ^ (h2 << 1);
        }
    };
} // namespace std
