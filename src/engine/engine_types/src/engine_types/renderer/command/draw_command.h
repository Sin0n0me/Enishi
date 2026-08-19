#pragma once
#include <cstdint>
#include <variant>

namespace enishi::types {
    enum class SubCommand {
        Bind,   // リソースのバインド
        Unbind, // リソースのアンバインド
        Nop,    // 何も実行しない(デバッグ用など)
        Clear,
    };

    struct DrawCommand {
        RenderHandle handle;
        SubCommand sub_command;
    };
} // namespace enishi::types
