#pragma once
#include <cstdint>

namespace enishi::types {
    enum class AssetState : std::uint8_t {
        NotLoaded, // 初期状態(キュー未投入, または解放済み)
        Queued,    // スレッドプール等の処理待ちキューに入っている状態
        Loading,   // バックグラウンドスレッドで読み込み中
        Loaded,    // 読み込み完了
        Failed     // エラーにより読み込み失敗
    };
}