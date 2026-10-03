#pragma once

namespace enishi::model_controller {
    enum class ControlError {
        NotFound,   // 指定した名前のモデルが見つからない
        LoadFailed, // アセットの読み込みに失敗した
        BuildFailed // 描画データへの変換に失敗した
    };

    enum class BuildError {
        AssetNotReady, // モデルデータがまだ取得できない
        CreateMeshFailed,
        CreateTextureFailed,
    };
} // namespace enishi::model_controller
