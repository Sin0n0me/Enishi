#pragma once
#include <engine_types/skinning/skinning_command.h>
#include <glm/glm.hpp>
#include <vector>

namespace enishi::component {
    // 各ボーンの最終的なスキニング行列(= global * bind_global_inverse)
    // GPUのボーン用バッファへ転送する直前のデータであり、SkinningSystemが毎フレーム書き込む
    struct SkinningComponent {
        std::vector<glm::mat4> skinning_matrices;

        // 空の場合はSkinningSystemの既定順序(Animation -> IK -> PhysicsSimulate)が使われる
        // モデルによって順序を変えたい場合はここへ明示的に指定する
        std::vector<types::SkinningCommand> order;
    };
} // namespace enishi::component
