#pragma once
#include "../handle/renderer/render_handle.h"
#include "uniform_buffer/camera.h"
#include <engine_types/renderer/command/draw_command.h>
#include <glm/glm.hpp>

namespace enishi::types {
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