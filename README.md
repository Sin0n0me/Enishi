# Enishi

## ビルド

`src/CMakePresets.json` でOS・描画方式・コンパイラ・Debug/Releaseを選択します。以下の準備はプロジェクトルートで実行してください。

### 必要なツールと依存関係

- CMake 3.24以降、Git、Python 3、Rust/Cargo。
- WindowsのClang構成：Clang、Ninja、Visual StudioのC++ビルドツールとWindows SDK。x64のDeveloper PowerShellを使用し、`clang`、`clang++`、`ninja`をPATHに追加してください。
- Visual Studio構成：Visual Studio 2022または2026のC++ビルドツールとWindows SDK。
- GCC構成：GCCとNinja。WindowsではMinGW-w64を使用します。
- Windowsの依存パッケージ管理にはvcpkgを使用します。`VCPKG_ROOT`にvcpkgのルートを指定してください。

```powershell
git submodule update --init --recursive
python -m pip install -r .github/requirements-ci.txt
$env:VCPKG_ROOT = 'C:/tools/vcpkg'
```

Rustのバージョンは `src/engine/script/rust-toolchain.toml` の `channel` に合わせて導入してください。WindowsのGCC構成には、同じツールチェーンの `x86_64-pc-windows-gnu` ターゲットも必要です。

ICUは `src/vcpkg.json` の固定baselineと機能定義に従い、Windows構成のconfigure時に導入されます。Clang・Visual Studioでは `x64-windows`、GCCでは `x64-mingw-dynamic` を使用します。SDLなどの依存ソースはCMakeのFetchContent、Musubuなどはサブモジュールで管理しています。初回configure・ビルドにはネットワーク接続が必要です。

### Clangでビルドする

次のコマンドはWindowsのDirectX Debug構成です。

```powershell
cd src
cmake --list-presets
cmake --preset windows-directx-clang-debug
cmake --build --preset windows-directx-clang-debug --parallel 4
```

生成先は `build/presets/windows-directx-clang-debug` です。実行時の設定ファイルは作業ディレクトリから参照するため、プロジェクトルートへ戻って起動します。

```powershell
cd ..
./build/presets/windows-directx-clang-debug/enishi_engine.exe
```

preset名は `<OS>-<描画方式>-<コンパイラ>-<構成>` です。configureとbuildには同じ名前を指定してください。

| 選択項目 | 値 |
| --- | --- |
| OS | `windows`、`linux`、`macos` |
| 描画方式 | `directx`、`opengl`、`vulkan` |
| コンパイラ | `clang`、`gcc`、`visualstudio`（2022）、`visualstudio2026` |
| 構成 | `debug`、`release` |

DirectXとVisual StudioはWindows専用です。Linux/macOSではClangまたはGCCを使用し、ICUをCMakeから検索できる環境に準備してください。これらのpresetにはvcpkgツールチェーンを自動指定していません。

たとえば、`src`ディレクトリで次のように切り替えられます。

```powershell
cmake --preset windows-directx-visualstudio2026-release
cmake --build --preset windows-directx-visualstudio2026-release --parallel 4

cmake --preset windows-opengl-gcc-debug
cmake --build --preset windows-opengl-gcc-debug --parallel 4
```

各presetは別のビルドディレクトリを使用します。Visual Studioの実行ファイルはその中の `Debug` または `Release` ディレクトリ、Clang/GCCではビルドディレクトリ直下に出力されます。

### シェーダーとVulkan SDK

描画ロジックは `src/shader/hlsl` とその `common` 配下で管理し、アプリのビルド時にシェーダーを生成します。

- DirectX：Windows SDKのFXCでDXBCへコンパイル。
- Vulkan：Vulkan SDKのDXCでSPIR-Vへコンパイル。
- OpenGL：DXCでSPIR-Vへコンパイルし、SPIRV-CrossでGLSL 4.0へ変換。

OpenGLとVulkanにはVulkan SDKの `dxc`、`spirv-val` が必要です。Vulkan構成ではヘッダー・ローダー、`glslc`、`Source/SPIRV-Reflect`も使用します。`VULKAN_SDK`にSDKのルートを設定し、必要なツールをPATHへ追加してください。

```powershell
$env:VULKAN_SDK = 'C:/VulkanSDK/<インストールしたバージョン>'
$env:PATH = "$env:VULKAN_SDK/Bin;$env:PATH"
# srcディレクトリで実行
cmake --preset windows-vulkan-clang-debug
cmake --build --preset windows-vulkan-clang-debug --parallel 4
```

生成シェーダーは各ビルドディレクトリの `shader/<描画方式>` に出力します。Vulkanの実行環境にはVulkan 1.1以上に対応するGPUドライバーと、64 KiB以上のuniform buffer範囲が必要です。

### Presetsの更新と検証

構成の追加・除外は `tools/generate_cmake_presets.py` を編集し、プロジェクトルートで再生成してください。生成したJSONもcommitします。CIは `--check` で再生成漏れを検出します。

```powershell
python tools/generate_cmake_presets.py
python tools/generate_cmake_presets.py --check
```

環境固有の設定にはGit管理対象外の `src/CMakeUserPresets.json` を使用できます。

全構成の一括ビルドと回帰テストは、プロジェクトルートから実行できます。

```powershell
python tools/build_all.py windows --compiler clang --jobs 4
python tools/test_all.py --engine-build build/presets/windows-directx-clang-debug --jobs 4
```

回帰テストにはconfigure済みのエンジンビルドと導入済みの依存関係が必要です。`test_all.py` は外部取得を無効にして実行します。

Vulkanの描画テストのみを実行する場合は、`src`ディレクトリで次のように指定します。

```powershell
cmake --preset windows-vulkan-clang-debug -DENISHI_VULKAN_TESTS=ON
cmake --build --preset windows-vulkan-clang-debug --target vulkan_render_tests --parallel 4
$env:VK_LAYER_PATH = "$env:VULKAN_SDK/Bin"
ctest --test-dir ../build/presets/windows-vulkan-clang-debug -C Debug -R vulkan --output-on-failure
```

CIの成功したビルドは、GitHub Actionsの実行ページにあるArtifactsから取得できます。exe・DLL・PDB・シェーダー・設定ファイルを構成ごとにまとめ、14日間保持します。

## 設定ファイル

カメラ操作・カーソル追従・クリック動作は `config/settings.cfg` にまとめて設定します。名前の辞書は `config/bone_names.toml` と `config/morph_names.toml` に分離しています。

```text
[camera]
invert_wheel off

[cursor_tracking]
neck_yaw 10

[model_events]
"*" "click" "*" impulse 0 0 5.0 on
```

各設定は対応するセクション内に記述し、同じセクションを複数回定義しないでください。未指定の項目は既定値を使います。不正な記述がある場合は行番号をログに出し、統合ファイル全体を適用せず既定値を使います。ボーン辞書・モーフ辞書の読み込みには影響しません。

モーションの相対パスは設定ファイルのある `config` ディレクトリが基準です。

### モーフ名の辞書

`config/morph_names.toml` は全モデル共通の辞書です。UTF-8のTOML形式で、形式のバージョンと役割ごとの候補名を記述します。

```toml
version = 1

[roles.blink]
aliases = ["custom_blink", "まばたき", "blink"]

[roles.mouth_a]
aliases = ["custom_mouth_a", "あ"]
```

候補は左から順に、大文字小文字を区別して完全一致で検索します。記述した役割の既定候補は置き換えられ、未指定の役割は既定値を使います。役割一覧は設定ファイル内のコメントを参照してください。変更は再起動後に反映されます。構文エラー、未知のキー・役割、空の候補や重複定義があれば位置をログへ出し、その辞書全体を既定値に戻します。`version = 1` は必須です。

ボーン辞書 `config/bone_names.toml` も同じ形式です。たとえば首の候補は次のように設定します。

```toml
version = 1

[roles.neck]
aliases = ["custom_neck", "neck", "Neck"]
```

コードからは `animation::MorphRole` と `animation::MorphNameDictionary` で検索できます。起動時に読み込んだ辞書を使う場合は、`AnimationSystem::find_morph(entity, animation::MorphRole::Blink)` でモデル内のモーフ番号を取得します。該当するモーフがなければ `types::INVALID_MORPH_INDEX` を返します。この辞書の追加だけで表情を自動再生することはありません。


## クリック時のモーション設定

`config/settings.cfg` の `[model_events]` セクションを UTF-8 で編集し、アプリケーションを再起動すると設定が反映されます。
選択UIの「クリック設定」で「触る」を選ぶとモーションを再生し、「押す」を選ぶと衝撃を加えます。
「なし」では何もしません。初期設定は「押す」で、モーションの割り当てはありません。

モーションを割り当てるには、次の形式の行を追加してください。

```text
"モデル名またはパスの一部" "click" "ボーン名または*" motion "VMDファイルのパス" once
```

例えば、対象モデルのどの部位のクリックでもモーションを1回再生する設定は次のとおりです。
以下のモデル名とファイル名は説明用の仮の値です。使用するモデル名とVMDファイルのパスに置き換えてください。初期設定には含まれません。

```text
"sample_model" "click" "*" motion "../motions/sample_motion.vmd" once
```

- モデルの指定はモデル名または読み込み元パスの部分一致です。`"*"` は全モデルに一致します。
- ボーン名は完全一致です。`"*"` はどの部位にも一致します。特定のボーンを指定すると、そのボーンの当たり判定をクリックしたときだけ発火します。再生対象をそのボーンだけに制限する設定ではありません。
- VMDの相対パスはconfigファイルのあるディレクトリを基準に解決します。パスの区切りには `/` を使用してください。
- `once` または `on` は1回再生、`loop` は繰り返し再生、`off` は無効です。無効な行のモーションファイルは読み込みません。
- 「触る」ではモーションの行、「押す」では `impulse` の行が選ばれます。両方を同じconfigに記述できます。
- 一致する有効な行が複数ある場合は、ボーン指定、モデル指定の順で優先され、同じ優先度なら先の行が使われます。アクションを `none` にした行で、特定のモデルや部位の動作を抑制できます。
- `#` で始まる行はコメントです。

初期設定の次の行は、「押す」で全モデルを対象にワールド座標のZ方向へ衝撃を加えます。

```text
"*" "click" "*" impulse 0 0 5.0 on
```

`impulse X Y Z on|off` の数値は継続的な力ではなく瞬間的な衝撃量です。
必要に応じてモデルの質量に合わせて調整してください。
反応対象のモーションステートは、例えば次の行で絞り込めます。

```text
push_states Dynamic DynamicAdjustBone
```

省略時または `push_states all` では全ステートが判定対象です。`push_states none` では対象がなくなります。
`Kinematic` はクリック判定の対象にできますが、アニメーションで駆動されるため物理的な衝撃では動きません。

## カーソルへの反応

モデルの近くにカーソルを置くと、首・頭がその方向を向き、追従範囲内の両手を伸ばします。
左右の手は個別に範囲を判定します。胴体付近の重なる範囲では両手、片方の範囲外では届く側の手だけが追従します。
範囲は肩からの距離（上腕と前腕の長さの合計）と、反対側の肩までの横方向の広がりで判定します。
境界付近には追従を維持する余裕を設け、範囲外になった手は速度制限を守って元の姿勢へ戻します。
クリックや専用のモーションファイルは不要です。カーソルが離れると元の姿勢へ滑らかに戻ります。
再生中のアニメーションに回転を重ね、既存のIK・物理演算より前に適用します。
視線は顔を通るカメラ向きの平面で計算し、顔の中心付近では首・頭の追従回転をゼロへ戻します。
中心は左右の目ボーンの中点を使い、目ボーンがない場合は首・頭の位置から推定します。追従前の姿勢を基準にするため、追従自体で目標がずれ続けることはありません。
首・頭の角度と腕の到達距離には制限があり、ボーンの長さは変更しません。
`config/settings.cfg` の `[cursor_tracking]` セクションで追従の制限を設定できます。変更はアプリの再起動後に反映され、全モデルに適用されます。

| キー | 初期値 | 意味・設定範囲 |
| --- | --- | --- |
| `neck_yaw` | `10` | 首の左右の追加回転の上限（度、0～90） |
| `neck_pitch` | `6` | 首の上下の追加回転の上限（度、0～90） |
| `head_yaw` | `25` | 頭の左右の追加回転の上限（度、0～90） |
| `head_pitch` | `15` | 頭の上下の追加回転の上限（度、0～90） |
| `arm_speed` | `90` | 肩・ひじの追従回転速度の上限（度/秒、0より大きく720以下） |
| `experimental_obb_collision` | `off` | 実験的なOBB干渉回避（`on` / `off`） |
| `jump_enabled` | `on` | 上方のカーソルへその場でジャンプする（`on` / `off`） |
| `jump_height_ratio` | `0.18` | ジャンプ高さの上限／ボーン全体の高さ（0～0.5） |
| `jump_duration` | `0.65` | 離陸から着地までの秒数（0.2～10） |
| `jump_delay` | `0.35` | 届かない上方のカーソルを追ってから発動するまでの秒数（0～10） |
| `jump_cooldown` | `1.5` | 着地後、次の発動待ちを始めるまでの秒数（0～10） |

各行は `キー 値` 形式で、`#` 以降はコメントです。省略したキーには初期値を使います。
例えば `arm_speed 45` で腕をよりゆっくり動かし、`neck_pitch 3` で首の上下の動きを小さくできます。
角度を0にするとその軸の追従を止めます。首と頭の回転は親子関係で合成されます。
ジャンプは、通常は手が届かず、上方向へ移動すれば届くカーソルに対して発動します。横移動は行いません。
左右の脚を認識できる場合は、踏み切り前の0.24秒と着地後の0.20秒に膝を軽く曲げます。足先・足IKの目標を保ったまま腰を下げ、着地後は元の姿勢へ戻します。脚を認識できない場合は上下移動のみを行います。
必要な高さを腕の長さから求め、設定した上限内で放物線を描いて上昇・下降します。飛行中にカーソルが離れても着地まで続けます。
全ルートボーン（足IKの目標を含む）へ移動を加えてからIK・物理演算へ渡し、元のモーションの位置には蓄積しません。地形との着地判定ではなく、元のモーション位置に戻る動作です。
腕の速度制限は伸ばすとき・戻すとき・左右の切り替え時に適用します。元のモーションや物理演算の速度は変更しません。
元のモーションや頭の動きで前フレームの腕が体内に入った場合は、貫通回避を速度制限より優先します。
ファイルがない場合や不正な設定（未知のキー、重複、範囲外など）がある場合は、警告をログに出し、設定全体を初期値に戻します。

ひじはモデル基準の軸で0～135度の範囲に曲げ、肩の開きとねじれも制限します。
胴体・頭や両腕の簡易カプセルによって干渉を避けます。
ボーン別のOBBによる干渉回避は実験的機能で、初期状態では無効です。`config/settings.cfg` の `[cursor_tracking]` セクションに `experimental_obb_collision on` と設定し、再起動すると有効になります。`off` またはキーの省略で無効になります。
無効時は追従用OBBの構築・更新・判定とOBBによる手の大きさの推定を省きます。モデルのクリック判定には影響しません。
有効時はボーン別の頂点群から作った既存のOBBを使い、手・指・前腕と衣服などの干渉も避けます。
両手の目標を手の大きさに応じて左右に離し、反対側の手・前腕との干渉も判定します。接触する目標は前方・外側へずらし、補間・速度制限後にも確認します。
自分の腕に付属する袖や関節の接続部分は自己衝突の対象から除外します。物理演算で動く衣服のOBBには直前に確定した物理姿勢を使います。
OBBの判定候補から直接の親子ボーンを除外します。祖先・子孫全体を除外するわけではなく、胴体と手などの判定は維持します。
判定するボーンの組を事前に絞り、変更された姿勢と必要なOBBだけを更新します。遠い形状は簡易判定で除外し、重なり判定では接触点の生成を省きます。
OBBは形状の近似であり、変形したメッシュの三角形との厳密な衝突判定ではありません。後段のIK・物理演算による姿勢変化まで完全に保証するものではありません。

MMDの前方（-Z）を基準に、次のボーン名を自動認識します。

| 部位 | 対応するボーン名 |
| --- | --- |
| 首・頭 | `首` / `neck` / `Neck`、`頭` / `head` / `Head` |
| 左腕 | `左腕` / `leftUpperArm` / `LeftArm`、`左ひじ` / `左肘` / `leftLowerArm` / `LeftForeArm`、`左手首` / `leftHand` / `LeftHand` |
| 右腕 | `右腕` / `rightUpperArm` / `RightArm`、`右ひじ` / `右肘` / `rightLowerArm` / `RightForeArm`、`右手首` / `rightHand` / `RightHand` |

腕は上腕→ひじ→手首の親子関係が必要です。途中に捩りボーンがあっても認識します。
認識できない部位、物理演算で制御されるボーン、回転軸が固定されたボーンは補正しません。
非表示のモデルは更新しません。

assetsを使わない検証は `-DENISHI_CURSOR_TESTS=ON` で構成し、`enishi_cursor_tests` をビルドしてから
`ctest --test-dir <ビルド先> -C Debug -R enishi_cursor_tests --output-on-failure` で実行できます。
`enishi_cursor_tests --benchmark` は約200ボーンの人工モデルで300フレームの追従処理を計測します。Release構成を使い、初期化とウォームアップの時間は計測に含めません。

## スクリプト

Musubuは `src/engine/script/Musubu` にサブモジュールとして固定されています。
初回取得時は `git submodule update --init --recursive` を実行してください。
ビルドには `src/engine/script/rust-toolchain.toml` で指定したRustツールチェーンとCargoが必要です。
CMakeがMusubuの接続ライブラリをビルドするため、別途DLLをコピーする必要はありません。
依存関係がキャッシュ済みの場合は `-DENISHI_CARGO_OFFLINE=ON` でCargoの外部取得を無効にできます。

選択UIの「スクリプト」を押すと、入力欄・エラー欄・実行ボタン・戻るボタンを表示します。
「実行」は入力全体をコンパイルし、引数のない `main()` を呼び出します。
例えば次の入力は整数42を返します。

```rust
fn main() -> i32 {
    6 * 7
}
```

コンパイルエラーは行・列情報を含めて表示し、実行時エラーも同じエラー欄に表示します。
入力は画面を戻っても保持されます。実行はUIスレッド上で同期的に行うため、終了しないループは使用しないでください。

各システムは `sub_system::IScriptResultReceiver` を実装し、共有する
`sub_system::IScriptEngine` の `add_result_receiver()` に登録することで結果を受け取れます。
`on_script_result(const ScriptValue&)` は実行成功時に登録順で呼ばれます。
戻り値は型付きの所有データで、構造体のフィールド名や行列の列構造も保持します。
受信側で必要な値を検査してシステムへ反映してください。登録先の寿命は呼び出し元が管理します。
受信側がエラーを返した場合は以後の通知を停止しますが、既に反映した変更は巻き戻しません。

assetsを使わないスクリプト実行・UI遷移・画面端配置の検証は、構成時に
`-DENISHI_SCRIPT_TESTS=ON` を指定し、`enishi_script_tests` をビルドしてから
`ctest --test-dir <ビルド先> -C Debug -R enishi_script_tests --output-on-failure` で実行できます。

### カメラの更新API

`Application::get_camera_controller()` は共有の `sub_system::ICameraController` を返します（`init()` 成功後に取得）。外部システムにはこのインターフェイスを渡し、具象クラスに依存せずカメラを更新できます。

- `get_camera()`：現在の行列のコピーを取得します。
- `set_view_projection(view, projection)`：ビュー・射影行列を同時に更新します。既存のモデルカメラと同じ左手座標系の行列を指定してください。
- `reset()`：初期カメラへ戻します。

更新はアプリケーションスレッドのフレーム更新開始前に行います。MVPは `projection * view * world` で再計算されます。非有限値や逆行列を求められない行列はエラーになり、以前の状態を維持します。戻り値のエラーを呼び出し側で処理してください。
描画時のCamera定数バッファ、クリック判定、カーソル追従は同じ共有カメラを参照します。新しく表示したモデルや再表示したモデルにも、描画前に現在のカメラを転送します。

ホイールを上に回すとカメラが向いている方向へ前進し、下に回すと後退します。視野角は変更しません。移動量は1目盛りにつき1モデル単位で、タッチパッドなどの小数のスクロール量も反映します。UIがホイール操作を受け取っている間はズームしません。
方向を反転するには `config/settings.cfg` の `[camera]` セクションを次のように設定し、再起動してください。

```text
invert_wheel on
```

初期値の `off` に戻すと通常方向になります。`#` 以降はコメントです。キーを省略した場合は初期値を使い、ファイルがない場合や不正な設定がある場合は警告をログに出して初期値を使います。
