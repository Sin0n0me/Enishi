#pragma once
#include "errors/errors.h"
#include <condition_variable>
#include <engine_types/assets/asset_data.h>
#include <engine_types/handle/asset/asset_handle.h>
#include <filesystem>
#include <foundation/option/option.h>
#include <foundation/result/result.h>
#include <foundation/thread/single_thread_executor.h>
#include <functional>
#include <mutex>
#include <unordered_map>

namespace enishi::assets_system {
    /**
     * この型の目的
     *
     * アセットの非同期読み込み(IO)専用スレッドの管理と、その完了結果の受け渡しのみに責務を絞ったクラス
     * 使用可能なIO専用スレッドは1本のみという制約のもと、foundation::SingleThreadExecutorを介して
     * ジョブを直列実行し、完了結果をAssetHandle単位で保持する
     *
     * ECS(Registry)への登録や、AssetState(Queued/Loading/Loaded/Failed)の最終確定はこのクラスの
     * 責務ではない。呼び出し元(AssetManager)が完了結果を取り出したうえで行う
     *
     * このクラス自体は「読み込み中データの一時置き場」以上の意味を持たないため、
     * メンバ関数はすべて非const(素直にミュータブル)である
     * 呼び出し元がconstメソッドの中からこのクラスを操作したい場合は、
     * 呼び出し元側でこのクラスのインスタンスをmutableにして保持することを想定している
     */
    class AssetLoadScheduler {
      public:
        // IOスレッドで実行するジョブ本体(ファイルの読み込み処理そのもの)
        using LoadJob = std::function<foundation::Result<types::AssetData, AssetError>(void)>;

        // IOスレッドでの読み込みが完了した1件分の結果
        struct CompletedLoad {
            types::AssetHandle handle;
            std::filesystem::path path;
            foundation::Result<types::AssetData, AssetError> result;
        };

      private:
        foundation::SingleThreadExecutor io_executor;

        std::mutex completed_mutex;
        std::condition_variable completed_condition;
        std::unordered_map<types::AssetHandle, CompletedLoad> completed_loads;

      public:
        AssetLoadScheduler(void) = default;
        ~AssetLoadScheduler(void) noexcept = default;

        AssetLoadScheduler(const AssetLoadScheduler&) = delete;
        AssetLoadScheduler& operator=(const AssetLoadScheduler&) = delete;
        AssetLoadScheduler(AssetLoadScheduler&&) = delete;
        AssetLoadScheduler& operator=(AssetLoadScheduler&&) = delete;

        // ジョブをIO専用スレッドへ積む。呼び出し元のスレッドをブロックしない
        void submit(
            const types::AssetHandle& handle, const std::filesystem::path& path, LoadJob job);

        // 完了済みの結果を1件取り出す(非ブロッキング)。結果が無ければNoneを返す
        // フレーム毎のポーリング等、複数件をまとめて処理したい場合に繰り返し呼ぶことを想定している
        [[nodiscard]] foundation::Option<CompletedLoad> try_take_completed(void) noexcept;

        // 指定ハンドルの完了を待って結果を取り出す(ブロッキング)
        //
        // 前提条件:
        // 呼び出し時点でhandleに対応するジョブが投入済みで、まだ結果を取り出していないこと
        // 呼び出し元(AssetManager)は状態(AssetState)が読み込み中であることを確認したうえでのみ
        // このメソッドを呼ぶ責務を負う。そうでない場合は完了が来ず無限に待機し続けてしまう
        [[nodiscard]] foundation::Option<CompletedLoad> wait_and_take_completed(
            const types::AssetHandle& handle) noexcept;
    };
} // namespace enishi::assets_system
