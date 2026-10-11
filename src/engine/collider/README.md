# collider

モデルインスタンス単位でボーン別OBBを管理し、異なるモデル間の衝突を判定します。
同じアセットから生成したモデルでも、インスタンスごとに異なる `CollisionModelId` を指定してください。
モデル内のボーン同士は判定の組み合わせ自体を作りません。

## 利用手順

1. `Collider::register_model(model_id)` でモデルを登録します。
2. `set_bone(model_id, bone_index, positions, bone_to_world)` に、1ボーン分の頂点座標を渡します。
   頂点座標と変換行列の入力空間を揃えてください。通常はボーンローカル座標と、そのボーンのワールド行列です。
   モデルローカルのバインド姿勢の頂点を渡す場合は、バインド姿勢から現在のワールド空間への変換を渡します。
3. `add_handler(model_id, handler)` で `ICollisionHandler` の実装を登録します。
4. 各フレームで `update_bone(model_id, bone_index, bone_to_world)` により姿勢を更新し、
   全モデルの更新が完了してから `check_collisions()` を呼びます。

```cpp
class CollisionHandler : public enishi::sub_system::ICollisionHandler {
  public:
    void on_collision(const enishi::types::BoneIndex bone_index,
        const enishi::types::Collision& collision) override {
        // bone_index: このハンドラーを登録したモデル側のボーン
        // collision.other_model / other_bone: 相手モデルとボーン
        // collision.contact.position: ワールド空間の衝突位置
        // collision.contact.normal: 自分から相手へ向かう法線
        // collision.contact.penetration_depth: 分離に必要な最小移動量
    }
};
```

`set_bone` は同じボーンを再登録するとOBBを置き換えます。`update_bone` は保存した元のOBBから
毎回計算するため、変換を累積しません。回転、移動、非一様スケール、反転に対応し、
せん断がある場合は変換後の箱を包むOBBに再フィットします。
`get_obb_map` からワールド空間のOBBマップを参照できます（モデル削除後は参照無効）。

空の頂点群、非有限の座標、無効なボーンインデックス、非アフィン行列などは失敗として返します。
モデル未登録の場合も操作は失敗します。戻り値を確認してください。失敗した更新では既存のOBBを保持します。
頂点群を渡す段階でボーンへの割り当ては完了している前提で、スキニングウェイトからの頂点分類は行いません。

## 判定と通知

モデル全体のAABBで候補を絞り、OBB同士を15軸の分離軸テストで判定します。
面・辺・頂点の接触も衝突に含め、同じボーンペアの衝突は1回のチェックで1度だけ処理します。
接触が続く場合はチェックのたびに双方の登録済みハンドラーへ通知します。
`check_collisions()` の戻り値は衝突したボーンペア数で、通知回数ではありません。

位置は両OBBの辺をクリップして得た重なり領域内の代表点です。
メッシュ表面の厳密な接触点や接触面全体ではありません。浮動小数点の丸め誤差を含み、
ほぼ共面の接触位置の算出では箱の大きさに対して最大 `1e-6` の許容幅を使用します。
中心が一致する場合など、分離方向が一意でないケースではいずれかの有効な法線を返します。
判定はその時点の姿勢に対する離散判定で、フレーム間のすり抜け検出や物理応答は行いません。

ハンドラーは `shared_ptr` で保持します。`remove_handler` / `remove_model` で登録を解除できます。
通知内容とハンドラーの寿命はチェック開始時の状態に基づいて確保されるため、
コールバック内の登録・削除・姿勢変更は次回のチェックから反映されます。
通知順序は未規定です。単一スレッドで使用し、コールバックから `check_collisions()` を再帰呼び出ししないでください。

旧版の未完成なカメラ／マウス座標ベースの `Collider` APIは、このモデル登録APIに置き換えています。
単体のレイ判定は `Collider::hit_model(ray, obb)` で利用できます。
アプリケーション側では、ボーン更新後に上記APIを呼び出して接続してください。

## テスト

既存のGLMとICUが利用できる環境で、`collider` を独立して構成できます。
新しい外部依存はありません。`BUILD_TESTING` は他プロジェクトのテスト設定と共通なので、
このプロジェクトのテストには `COLLIDER_BUILD_TESTING` を使用します。

```text
cmake -S src/engine/collider -B build/collider -DCOLLIDER_BUILD_TESTING=ON -DBUILD_TESTING=OFF
cmake --build build/collider --config Debug --target collider_test
ctest --test-dir build/collider -C Debug --output-on-failure
```

外部取得を禁止する環境では、構成時に `FETCHCONTENT_FULLY_DISCONNECTED=ON` と
`FETCHCONTENT_SOURCE_DIR_GLM=<既存のGLMソース>` を指定し、必要に応じて `ICU_ROOT` を指定してください。
テストはOBB生成、退化した頂点群、姿勢更新、接触位置、自己衝突除外、継続通知、通知中の削除に加え、
12半空間の交差から独立に求めた結果と500組の回転OBBの判定を比較します。
