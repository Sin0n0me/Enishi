#pragma once
#include "render_handle.h"
#include "uniform_buffer/camera.h"
#include <glm/glm.hpp>

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

    struct RenderPass {
        RenderHandle render_target; // NullHandle でスワップチェーンに描く
        std::vector<DrawCommand> commands;
    };

    // フレーム全体の描画記述
    struct RenderGraph {
        std::vector<RenderPass> passes; // 順番通りに実行される
        UniformCamera camera;           // 全パス共通のカメラ情報
    };
} // namespace enishi::types