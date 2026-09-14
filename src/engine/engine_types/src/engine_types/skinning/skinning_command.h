#pragma once
#include <cstdint>
#include <engine_types/skinning/skinning_command.h>
#include <vector>

namespace enishi::types {
    enum class SkinningCommand : std::uint8_t {
        ReadBoneMatrices,         // bone_matrixからglobalへ読み込み
        WriteBoneMatrices,        // globalからbone_matrixへ書き込み
        UpdateLocal,              // transformを合成してlocalに書き込み
        UpdateGlobal,             //
        ResetLocalTransform,      //
        ResetPosition,            //
        ResetRotate,              //
        ResetScale,               //
        Animation,                // アニメーションを適用した結果を現在のtransformと合成
        PhysicsSimulate,          // 物理演算を適用した結果を現在のtransformと合成
        WriteBackPhysicsSimulate, // 現在のglobalを物理エンジンに書き戻し
        IK,                       // IKシステムを適用した結果を現在のtransformと合成
    };

    // 回転の合成の順序
    enum class MultiplyOrder : std::uint8_t {
        Animation,
        IK,
        Pthisics,
    };
} // namespace enishi::types