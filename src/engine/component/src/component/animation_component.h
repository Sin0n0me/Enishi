#pragma once
#include <cstdint>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <vector>

namespace enishi::component {
    enum class AnimationCommand : std::uint8_t {
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

    struct AnimationBuffer {
        glm::mat4 local;
        glm::mat4 global;
        glm::vec3 position;
        glm::vec3 scale;
        std::vector<glm::quat> rotations; // 計算順序によって変わるのでvecで保持
    };

    using BufferIndex = std::uint32_t;

    struct AnimationComponent {
        std::unordered_map<MultiplyOrder, BufferIndex> order_map;
        std::vector<AnimationCommand> commands;
        std::vector<MultiplyOrder> multiply_order;
        std::vector<AnimationBuffer> bone_buffer; // ボーンごとの中間データ
        std::vector<glm::mat4> bone_matrices;     // 最終結果(GPU送信用)
        float elapsed_time;                       // 経過時間
    };
} // namespace enishi::component
