# PMX の読み込み

`ModelLoader` は `.pmx` を `PMXModelLoader` で読み込み、`PMXToModelData` を通して
`types::AssetModelData` に変換します。PMD の登録と変換経路は維持しています。

## ローダ

- PMX 2.0 / 2.1、UTF-8 / UTF-16LE、1 / 2 / 4 バイトの参照番号に対応します。
- 頂点、面、テクスチャ、材質、ボーン、全モーフ形式、表示枠、剛体、全ジョイント形式、
  ソフトボディを `PMXData` に保持します。
- 未対応バージョン、不正な文字列・列挙値・件数、途中で切れたファイル、範囲外参照、
  材質の面数の不一致、ボーンやグループモーフの循環参照はエラーになります。
- フリップモーフの自己参照は仕様上許されているため、循環検査の対象外です。
- `PMXModelLoader::parse` はメモリ上のバイト列を読み込めます。

## 共通型への変換

共通型にはPMXの構造体、数値フラグ、表示パネル番号などを持ち込みません。

`PMXModelLoader` の構造検証と、`PMXToModelData` の実行機能の検査は別です。
次の機能を含むデータは生データとして読み込めますが、共通型への変換は
`AssetError::UnsupportedFeature` と対象機能を示すメッセージを返します。
検査はモデル生成・テクスチャ読み込みより前に行い、部分的なモデルは返しません。

- ボーンを持たないモデル、ウェイトが0でない未関連ボーン参照
- ボーン数が512を超えるモデル（未使用ボーンも含む）
- 頂点カラー、点・線材質、追加UVを使うスフィアのサブテクスチャ
- ボーン未関連の剛体、接続先剛体が欠けたジョイント、ばね付き6DOF以外のジョイント
- ソフトボディ

以下の表は共通型の表現を示します。型が存在するだけでは実行対応を意味しません。
上記の未対応機能を含むモデルを現在の変換APIから返すことはありません。

| 情報 | 共通型での表現 |
| --- | --- |
| 頂点ウェイト | 既存の `Skinning`、または32ビット参照の `Skinning4` |
| 変形方式 | `SkinningMethod::LinearBlend` / `DualQuaternion` / `SphericalBlend` |
| 球面補間 | `SphericalBlend` のバインド空間の中心と補正済みアンカー |
| 追加UV | 頂点順の `additional_uv_channels` |
| ボーン | 名前、親子関係、ローカル・グローバル・逆バインド行列 |
| ボーンの付与・制約 | `AddonBoneConstraints` の変形継承、軸、評価順序 |
| IK | `CCDIK` とチェーン順の `IKLinkLimit` |
| モーフ | `AddonMorphTargets` の頂点・UV・ボーン・材質属性・ウェイト・インパルス |
| 材質 | 色・テクスチャ・描画範囲・両面・影・輪郭・トポロジー |
| 剛体 | ボーン相対位置、形状、衝突対象マスク、物理パラメータ |
| ジョイント | 汎用の種類、制限、ばね、モーター |
| ソフトボディ | 形状、空力・ソルバー設定、アンカー、固定頂点 |

頂点レイアウトはモデル内で統一します。BDEF4/QDEFを含むモデルには `Skinning4` を使い、
4ウェイトの合計を1に正規化します。それ以外には既存の `Skinning` を使用します。
描画前に両形式を共通の4ウェイトGPU頂点へ変換するため、PMDや変形方式の混在にも対応します。
現行のOpenGL/HLSLシェーダの行列配列に合わせ、ボーン数の上限は512です。
構造検証で参照番号の範囲も確認するため、変換成功時の有効な参照番号は0〜511です。
この上限はウェイト0の参照にも適用され、32ビット参照を必要とするモデルも拒否します。
頂点転送は32ビット参照に対応しましたが、512ボーン制限は維持しています。
将来の行列転送・シェーダ容量の拡張は、上限チェックにTODOとして残しています。
面の参照番号は32ビットで保持します。共通の `BoneIndex` の無効値は型の最大値とし、
実在する65535番のボーンと区別します。
GPUへ渡すウェイト0の参照は0番ボーンへ置き換えます。球面補間やGPUの分岐評価で
未使用枠から行列を取得しても、無効な添字を渡さないためです。

頂点モーフは既存の `AddonMorphs` にも変換します。モーフ番号は元の順序を維持し、
頂点以外のモーフの頂点リストは空です。`AddonMorphTargets` を処理する利用側は、
互換用 `AddonMorphs` を重ねて適用しないでください。

材質モーフの属性名は `diffuse`、`specular`、`shininess`、`ambient`、`outline_color`、
`outline_width`、`base_color_texture_tint`、`environment_texture_tint`、
`shading_ramp_texture_tint` です。スカラーは `value.x`、3成分の色は `value.xyz` を使います。
全材質指定と未接続剛体の番号は `UINT32_MAX` で表現します。

テクスチャはモデルのディレクトリを基準にUTF-8パスを解決し、同一パスの読み込みを
重複させません。共有Toonもモデルのディレクトリにある `toon01.bmp` ～ `toon10.bmp` を
参照します。必要な画像がない場合や画像ローダが失敗した場合は、エラーを呼び出し元へ返します。

## 変換と実行の範囲

- ボーンは物理演算前・物理演算後の順に評価し、各区間で `evaluation_order` の昇順に
  変形継承とIKを処理します。同じ階層では元のボーン順を維持し、参照番号は変更しません。
  IKの補正は毎フレームのアニメーション姿勢から計算し、最終スキニング行列を描画用バッファへ反映します。
- IKリンク別の角度制限、回転・移動付与、負の付与率に対応します。
  固定軸・ローカル軸は操作用メタデータとして保持し、VMDの回転を制限しません。
- 外部親は `BoneConstraint::external_transform_slot` に変換します。
  `ModelComponent::external_transforms[slot]` にモデル空間の変換行列を設定すると、
  通常の親子変換の後に適用します。未設定・解除時は単位行列です。
- グループ・フリップから実効ウェイトを計算し、頂点・ボーン・UV・材質モーフを適用します。
  PMDのベースモーフ参照も頂点差分へ変換します。ウェイト0で元の値へ戻り、フレーム間で累積しません。
  ボーンモーフは付与・IKより前に適用します。
- UVは主UVと4個の追加チャンネルを更新してGPUへ転送します。
  現行シェーダは追加UVを使うスフィアサブテクスチャには対応しません。
- 材質モーフは乗算と加算を別々に集計し、材質ごとのuniformへ反映します。
  テクスチャの色係数・加算値と輪郭の色・幅も保持します。輪郭描画はDirectXの既存パスで反映し、
  OpenGLには輪郭描画パスがありません。
- インパルスモーフは剛体ごとに集計し、物理演算直前に適用します。
  ローカル空間の力とトルクは現在の剛体回転で変換します。速度リセットは集計した力の適用前に一度だけ行います。
  物理演算が無効な間はインパルスを蓄積しません。
- モデルの剛体・ジョイント定義は初期化時に登録し、動的な剛体の結果だけをボーンへ戻します。
  `PhysicsSystem` へ同じ `SkinningSystem` を渡すと、物理演算直後に物理後評価とスキニング行列の確定を行います。
- BDEF4は4行列の線形ブレンド、QDEFは符号を揃えて正規化する二重クォータニオン、
  SDEFは2回転の球面補間と補正済みアンカーの変換で実行します。位置と法線の双方が対象です。
- SDEFの生のC/R0/R1は `PMXData` に保持します。共通型には、中心と
  `anchor_i = C + (R_i - (w0*R0 + w1*R1))/2` で得たアンカーを渡します。
  描画位置は `slerp(q0,q1,w1)*(position-C) + w0*M0*anchor0 + w1*M1*anchor1` です。
  球面補間と二重クォータニオンは回転・平行移動からなるボーン変換を前提とします。
- OpenGLのモデル描画と、DirectXのモデル・エッジ・シャドウ描画に実装しています。
- 表示枠、英語名・コメント、ボーン表示用情報、ソフトボディのNear補正は
  PMX側のデータとして保持し、共通モデルの動作には変換しません。
- 拡張ジョイント、ソフトボディなどの未対応機能は上記の検査で拒否します。

## VMD再生

モデルをレジストリへ登録し `initialize_model_pose` を呼んだ後、
`AnimationSystem::play_vmd(entity, path, looping)` でVMDを再生できます。
ボーン名・モーフ名・IKボーン名をモデルへ対応付け、移動・回転・モーフウェイト・IK有効状態を更新します。
ファイルを読み込めなかった場合はエラーを返し、再生中のコントローラを置き換えません。
カメラ・照明・セルフシャドウのトラックはモデルアニメーションの再生対象に含みません。

手動でモーフを操作する場合は `MorphComponent::weights` を元のモーフ番号で更新します。
VMD再生中は同じウェイトをコントローラが毎フレーム更新します。

## 検証

既存のローカルGLMとICUヘッダを指定して、独立したテストをビルドできます。
このCMake構成は依存ライブラリを取得しません。

```powershell
cmake -G "NMake Makefiles" -S src/engine/assets_system/tests/pmx -B build/pmx-tests -DGLM_INCLUDE_DIR=<GLMヘッダの親ディレクトリ> -DICU_INCLUDE_DIR=<unicodeヘッダの親ディレクトリ>
cmake --build build/pmx-tests
ctest --test-dir build/pmx-tests --output-on-failure
```

テストでは生成バイナリを使い、文字コード・可変幅参照・全セクション・切断データ・
階層・ウェイト・モーフ・物理データ・テクスチャの変換とエラー伝播を確認します。
実モデルによる描画結果の検証は含みません。

Windows SDKの開発者環境では、描画側の独立テストも実行できます。既存のGLM/ICUのみを使用し、
新しい依存は取得しません。WARPで本番HLSLの変形関数を実行し、3描画パスのコンパイルと入力を検証します。

```powershell
cmake -G "NMake Makefiles" -S src/engine/renderer/tests/skinning -B build/skinning-tests -DGLM_INCLUDE_DIR=<GLMヘッダの親ディレクトリ> -DICU_INCLUDE_DIR=<unicodeヘッダの親ディレクトリ>
cmake --build build/skinning-tests
ctest --test-dir build/skinning-tests --output-on-failure
python src/engine/renderer/tests/skinning/gl_skinning_tests.py
```

最後のテストはWindowsの非表示WGLコンテキストで本番GLSLを実行し、座標と法線を読み戻します。
OpenGL 4.0対応ドライバが必要です。テスト対象は4ウェイト、単位変換、回転・平行移動、
クォータニオンの符号反転、SDEFアンカー、PMXからGPU頂点への変換とPMD形式の互換性です。

評価階層とIKの結合テストは、PMX変換からコンポーネント登録、階層更新、最終行列の転送までを確認します。
同値・負値を含む階層順、後方の親参照、複数IK、フレーム間の補正リセット、更新対象の混在を扱います。

```powershell
cmake -G "NMake Makefiles" -S src/engine/core/tests/bone_layers -B build/bone-layer-tests -DGLM_INCLUDE_DIR=<GLMヘッダの親ディレクトリ> -DICU_INCLUDE_DIR=<unicodeヘッダの親ディレクトリ>
cmake --build build/bone-layer-tests
ctest --test-dir build/bone-layer-tests --output-on-failure
```

材質モーフは `src/engine/renderer/tests/materials`、頂点・UV・モーフウェイトの計算は
`src/engine/core/tests/morphs`、VMD再生は `src/engine/animation/tests/runtime` の独立テストで検証できます。
VMD再生のテストには `ICU_UC_LIBRARY` と実行時のICU DLLも必要です。

物理演算とインパルスの統合テストは `src/engine/core/tests/physics` にあります。
GLM/ICUに加え、既存のBulletを `BULLET_INCLUDE_DIR`、`BULLET_DYNAMICS_LIBRARY`、
`BULLET_COLLISION_LIBRARY`、`BULLET_MATH_LIBRARY` で指定してください。
`CMAKE_BUILD_TYPE` は指定したBulletライブラリと同じ構成にします。このテストも依存の取得を行いません。

```powershell
cmake --build build/physics-runtime-tests
ctest --test-dir build/physics-runtime-tests --output-on-failure
```

## 参照資料

- [PMX仕様（PMXエディタ仕様書のミラー）](https://github.com/hirakuni45/glfw3_app/blob/master/glfw3_app/docs/PMX_spec.txt)
- [SabaのPMXローダ](https://github.com/benikabocha/saba/blob/master/src/Saba/Model/MMD/PMXFile.cpp)
- [SabaのSDEF/QDEF変形処理](https://github.com/benikabocha/saba/blob/master/src/Saba/Model/MMD/PMXModel.cpp)

仕様確認のみを行い、外部実装や新しい依存ライブラリは取り込んでいません。
