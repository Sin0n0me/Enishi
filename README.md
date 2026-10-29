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

Windowsではconfigure時に `src/vcpkg.json` の依存パッケージを導入します。

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

DirectXとVisual StudioはWindows専用です。LinuxではClangまたはGCC、macOSではClangとOpenGLを使用します。Linux/macOSにはICUの開発パッケージが必要です。

たとえば、`src`ディレクトリで次のように切り替えられます。

```powershell
cmake --preset windows-directx-visualstudio2026-release
cmake --build --preset windows-directx-visualstudio2026-release --parallel 4

cmake --preset windows-opengl-gcc-debug
cmake --build --preset windows-opengl-gcc-debug --parallel 4
```

各presetは別のビルドディレクトリを使用します。Visual Studioの実行ファイルはその中の `Debug` または `Release` ディレクトリ、Clang/GCCではビルドディレクトリ直下に出力されます。

### Linux / macOS

LinuxではClang 19以降とlibstdc++ 14以降、または同等のC++23標準ライブラリを使用してください。デスクトップへの配置とカーソル追跡にはX11を使用します。WaylandセッションではXWaylandが必要です。

```sh
cmake --preset linux-opengl-clang-debug -S src \
  -DCMAKE_C_COMPILER=clang-19 -DCMAKE_CXX_COMPILER=clang++-19
cmake --build build/presets/linux-opengl-clang-debug --parallel 4
./build/presets/linux-opengl-clang-debug/enishi_engine
```

macOSは14以降を対象とし、CocoaとOpenGL 4.1を使用します。`brew install llvm@21 ninja icu4c` で準備し、`$(brew --prefix llvm@21)/bin` をPATHへ追加してください。CIの `portable-opengl-shaders` 成果物を展開すると、macOS側でシェーダーコンパイラを準備せずビルドできます。

```sh
cmake --preset macos-opengl-clang-debug -S src \
  -DCMAKE_PREFIX_PATH="$(brew --prefix icu4c)" \
  -DCMAKE_EXE_LINKER_FLAGS="-L$(brew --prefix llvm@21)/lib/c++" \
  -DCMAKE_BUILD_RPATH="$(brew --prefix llvm@21)/lib/c++" \
  -DENISHI_PRECOMPILED_SHADER_DIRECTORY="/path/to/portable-opengl-shaders"
cmake --build build/presets/macos-opengl-clang-debug --parallel 4
./build/presets/macos-opengl-clang-debug/enishi_engine
```

変換済みシェーダーは現在のHLSLと変換スクリプトに一致するものが必要です。描画テストはconfigure時に `-DENISHI_OPENGL_TESTS=ON -DENISHI_EFFECT_RENDER_TESTS=ON` を指定し、`crack_render_tests` と `opengl_shadow_tests` をビルドしてから `ctest --test-dir <ビルド先> --output-on-failure` で実行します。

### シェーダーとVulkan SDK

OpenGLとVulkanのシェーダーをソースから変換する場合は、Vulkan SDKの `dxc`、`spirv-val` が必要です。Vulkan構成ではヘッダー・ローダー、`glslc`、`Source/SPIRV-Reflect`も使用します。`VULKAN_SDK`にSDKのルートを設定し、必要なツールをPATHへ追加してください。

```powershell
$env:VULKAN_SDK = 'C:/VulkanSDK/<インストールしたバージョン>'
$env:PATH = "$env:VULKAN_SDK/Bin;$env:PATH"
# srcディレクトリで実行
cmake --preset windows-vulkan-clang-debug
cmake --build --preset windows-vulkan-clang-debug --parallel 4
```

生成シェーダーは各ビルドディレクトリの `shader/<描画方式>` に出力します。Vulkanの実行環境にはVulkan 1.1以上に対応するGPUドライバーと、64 KiB以上のuniform buffer範囲が必要です。

### Presetsの更新と検証

構成の追加・除外は `tools/generate_cmake_presets.py` を編集し、プロジェクトルートで再生成してください。

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

回帰テストはビルド環境の準備後に実行してください。

CIの成功したビルドは、GitHub Actionsの実行ページにあるArtifactsから取得できます。Windowsはexe・DLL・PDB・シェーダー・設定ファイルを構成ごとにまとめ、14日間保持します。Linux/macOSは実行ファイル・シェーダー・設定ファイルのtar.gzとテスト結果を7日間保持します。tar.gzを展開したディレクトリで `./enishi_engine` を実行してください。ICUやC++標準ライブラリなどの共有ライブラリは含まないため、ビルド手順に記載した実行環境が必要です。

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

## クリック時のモーション設定

`config/settings.cfg` の `[model_events]` セクションを UTF-8 で編集し、アプリケーションを再起動すると設定が反映されます。
選択UIの「クリック設定」で「触る」を選ぶとモーションを再生し、「押す」を選ぶと衝撃を加えます。
「なし」では何もしません。初期設定は「押す」で、モーションの割り当てはありません。

モーションを割り当てるには、次の形式の行を追加してください。

```text
"モデル名またはパスの一部" "click" "ボーン名または*" motion "VMDファイルのパス" once
```

例えば、対象モデルのどの部位のクリックでもモーションを1回再生する設定は次のとおりです。
モデル名とVMDファイルのパスを置き換えてください。

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
カーソルが離れると元の姿勢へ滑らかに戻ります。
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

ボーン別のOBBによる干渉回避は実験的機能です。有効にするには、`[cursor_tracking]` に `experimental_obb_collision on` を設定して再起動してください。

MMDの前方（-Z）を基準に、次のボーン名を自動認識します。

| 部位 | 対応するボーン名 |
| --- | --- |
| 首・頭 | `首` / `neck` / `Neck`、`頭` / `head` / `Head` |
| 左腕 | `左腕` / `leftUpperArm` / `LeftArm`、`左ひじ` / `左肘` / `leftLowerArm` / `LeftForeArm`、`左手首` / `leftHand` / `LeftHand` |
| 右腕 | `右腕` / `rightUpperArm` / `RightArm`、`右ひじ` / `右肘` / `rightLowerArm` / `RightForeArm`、`右手首` / `rightHand` / `RightHand` |

腕は上腕→ひじ→手首の親子関係が必要です。途中に捩りボーンがあっても認識します。
認識できない部位、物理演算で制御されるボーン、回転軸が固定されたボーンは補正しません。
非表示のモデルは更新しません。

## スクリプト

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

## カメラ操作

ホイールを上に回すとカメラが向いている方向へ前進し、下に回すと後退します。視野角は変更しません。移動量は1目盛りにつき1モデル単位で、タッチパッドなどの小数のスクロール量も反映します。UIがホイール操作を受け取っている間はズームしません。
方向を反転するには `config/settings.cfg` の `[camera]` セクションを次のように設定し、再起動してください。

```text
invert_wheel on
```

初期値の `off` に戻すと通常方向になります。`#` 以降はコメントです。キーを省略した場合は初期値を使い、ファイルがない場合や不正な設定がある場合は警告をログに出して初期値を使います。
