#pragma once
#include "errors/errors.h"
#include <engine_types/handle/asset/asset_handle.h>
#include <engine_types/handle/renderer/render_handle.h>
#include <foundation/result/result.h>
#include <memory>
#include <platform/asset/interface_asset_system.h>
#include <platform/renderer/interface_renderer.h>
#include <vector>

namespace enishi::model_controller {
    /**
     * この型の目的
     *
     * 「読み込み済みのモデルデータを、描画で使える形(メッシュハンドル等)へ変換する」ことだけに
     * 責務を絞ったクラス
     *
     * ModelControllerのように「どのモデルを表示するか」といった操作(状態)は一切持たない
     * 呼び出されるたびに、渡されたモデルを変換して結果を返すだけの無状態なクラス
     */
    class ModelRenderDataBuilder {
      private:
        std::shared_ptr<platform::IRenderer> renderer;
        std::shared_ptr<platform::IAssetSystem> asset_system;

      public:
        ModelRenderDataBuilder(std::shared_ptr<platform::IRenderer> renderer,
            std::shared_ptr<platform::IAssetSystem> asset_system) noexcept;

        // model_handleが指すモデルデータを描画用メッシュへ変換する
        // モデルが参照するテクスチャの読み込みもここでまとめて要求する
        [[nodiscard]] foundation::Result<types::RenderHandle, BuildError> build(
            const types::AssetHandle model_handle,
            const std::vector<types::RenderHandle>& shader_reflections) noexcept;

      private:
        // モデルが参照する各テクスチャの読み込みを要求する(非同期。完了は待たない)
        void request_textures(const types::AssetModelData& model_data) noexcept;
    };
} // namespace enishi::model_controller
