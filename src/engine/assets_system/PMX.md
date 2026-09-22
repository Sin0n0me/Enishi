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

| 情報 | 共通型での表現 |
| --- | --- |
| 頂点ウェイト | 既存の `Skinning`、または32ビット参照の `Skinning4` |
| 変形方式 | `SkinningMethod::LinearBlend` / `DualQuaternion` |
| 追加UV | 頂点順の `additional_uv_channels` |
| ボーン | 名前、親子関係、ローカル・グローバル・逆バインド行列 |
| ボーンの付与・制約 | `AddonBoneConstraints` の変形継承、軸、評価順序 |
| IK | `CCDIK` とチェーン順の `IKLinkLimit` |
| モーフ | `AddonMorphTargets` の頂点・UV・ボーン・材質属性・ウェイト・インパルス |
| 材質 | 色・テクスチャ・描画範囲・両面・影・輪郭・トポロジー |
| 剛体 | ボーン相対位置、形状、衝突対象マスク、物理パラメータ |
| ジョイント | 汎用の種類、制限、ばね、モーター |
| ソフトボディ | 形状、空力・ソルバー設定、アンカー、固定頂点 |

頂点レイアウトはモデル内で統一します。4ウェイトまたは16ビットに収まらないボーン参照が
必要なモデルのみ `Skinning4` を使用し、それ以外は既存の `Skinning` を使用します。
面の参照番号は32ビットで保持します。共通の `BoneIndex` の無効値は型の最大値とし、
実在する65535番のボーンと区別します。

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

- SDEFは2ボーンの線形ブレンドへ変換します。SDEF固有の補正値は `PMXData` に残し、
  共通型に持ち込みません。このため変形結果はSDEFと完全には一致しません。
- 表示枠、英語名・コメント、ボーン表示用情報、外部親のキー、ソフトボディのNear補正は
  PMX側のデータとして保持し、共通モデルの動作には変換しません。
- この実装は読み込みと共通データへの変換です。既存の描画・アニメーション・物理処理に、
  `Skinning4`、二重クォータニオン、追加モーフ、追加制約、拡張ジョイント、ソフトボディを
  実行する処理は追加していません。それぞれの利用側で対応が必要です。

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

## 参照資料

- [PMX仕様（PMXエディタ仕様書のミラー）](https://github.com/hirakuni45/glfw3_app/blob/master/glfw3_app/docs/PMX_spec.txt)
- [SabaのPMXローダ](https://github.com/benikabocha/saba/blob/master/src/Saba/Model/MMD/PMXFile.cpp)

仕様確認のみを行い、外部実装や新しい依存ライブラリは取り込んでいません。
