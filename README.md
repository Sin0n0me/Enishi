# Enishi

## ビルド

プロジェクトルートの `build\build.cmd` で、レンダリングバックエンドとツールチェーンを指定してビルドできます。

`build\build.cmd` は生成ファイルです。変更する場合は
`build\generate_build.py` を編集してから、次のコマンドで再生成してください。

```bat
python build\generate_build.py
```

```bat
build\build.cmd <opengl|directx|vulkan> [Debug|Release|RelWithDebInfo|MinSizeRel] [auto|visualstudio|clang|gcc]
```

構成を省略した場合は `Debug`、ツールチェーンを省略した場合は `auto` を使用します。
`auto` は Visual Studio、Clang、GCC の順に利用可能なツールチェーンを選択します。

### Visual Studio

```bat
build\build.cmd opengl Debug
build\build.cmd directx Release
```

生成先はそれぞれ `build\opengl\visualstudio` と`build\directx\visualstudio` です。


Visual Studio ビルドでは、Visual Studio 18 2026 を優先して使用します。
18 が見つからない場合は Visual Studio 17 2022 を使用します。どちらも使用できない場合、
`auto` 指定では Clang、次に GCC へフォールバックします。

### Clang + Ninja

MSBuild を使わないビルドには Clang と Ninja を使用します。`clang` と`ninja` の両方が `PATH` から実行できる状態にしてください。

```bat
build\build.cmd opengl Debug clang
build\build.cmd directx Release clang
```

生成先はそれぞれ `build\opengl\clang` と `build\directx\clang` です。

### Vulkan

CMake 3.24以降と、[公式Vulkan SDK](https://vulkan.lunarg.com/sdk/home)が必要です。
SDKに含まれるVulkanヘッダー・ローダー、`glslc`、`Source/SPIRV-Reflect`を使用します。
実行環境にはVulkan 1.1以上に対応したGPUドライバーと、64 KiB以上のuniform buffer範囲が必要です。

```bat
build\build.cmd vulkan Debug
```

生成先は `build\vulkan\visualstudio`（Clang指定時は `build\vulkan\clang`）です。
Vulkan用GLSLはビルド時にSPIR-Vへ変換され、ビルドディレクトリの `shader\vulkan` から読み込まれます。
`assets` へのシェーダーのコピーや、`shader_compile.py` の実行は不要です。

SDKを通常のインストーラーで導入していない場合は、ビルド前に `VULKAN_SDK` をSDKのルートに設定してください。
今回の作業環境では `build\vulkan-sdk` にSDKを展開しているため、プロジェクトルートで次のように指定できます。

```bat
set "VULKAN_SDK=%CD%\build\vulkan-sdk"
build\build.cmd vulkan Debug
```

assetsを使用しない描画検証は、Vulkan構成に `-DENISHI_VULKAN_TESTS=ON` を追加してビルドすると実行できます。
検証時はSDKのValidation Layerを使用します。

```bat
cmake -S src -B build\vulkan\visualstudio -DUSE_VULKAN=ON -DENISHI_VULKAN_TESTS=ON
cmake --build build\vulkan\visualstudio --config Debug
set "VK_LAYER_PATH=%VULKAN_SDK%\Bin"
ctest --test-dir build\vulkan\visualstudio -C Debug --output-on-failure
```
### find_package のパッケージ探索

ビルド BAT は `find_package` の依存関係を探索するため、vcpkg の
`scripts\buildsystems\vcpkg.cmake` を CMake ツールチェインとして指定します。
`VCPKG_ROOT` を設定するか、`vcpkg.exe` を `PATH` に追加してください。

Visual Studio と Clang では `x64-windows`、GCC では `x64-mingw-dynamic` を
既定の vcpkg triplet として使用します。別の triplet を使う場合は、実行前に
`VCPKG_TARGET_TRIPLET` を設定してください。

vcpkg 以外のパッケージマネージャーを使用する場合は、
`CMAKE_TOOLCHAIN_FILE` にそのツールチェインファイルを設定できます。

### GCC + Ninja

GCC を明示的に使用する場合も、GCC と Ninja を `PATH` から実行可能にしてください。

```bat
build\build.cmd opengl Debug gcc
build\build.cmd directx Release gcc
```

生成先はそれぞれ `build\opengl\gcc` と `build\directx\gcc` です。

## クリック時のモーション設定

`config/model_event_motions.cfg` を UTF-8 で編集し、アプリケーションを再起動すると設定が反映されます。
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

モデルの近くにカーソルを置くと、首・頭がその方向を向き、近い側の腕を伸ばします。
クリックや専用のモーションファイルは不要です。カーソルが離れると元の姿勢へ滑らかに戻ります。
再生中のアニメーションに回転を重ね、既存のIK・物理演算より前に適用します。
視線は顔を通るカメラ向きの平面で計算し、顔の中心付近では首・頭の追従回転をゼロへ戻します。
中心は左右の目ボーンの中点を使い、目ボーンがない場合は首・頭の位置から推定します。追従前の姿勢を基準にするため、追従自体で目標がずれ続けることはありません。
首・頭の角度と腕の到達距離には制限があり、ボーンの長さは変更しません。
`config/cursor_tracking.cfg` で追従の制限を設定できます。変更はアプリの再起動後に反映され、全モデルに適用されます。

| キー | 初期値 | 意味・設定範囲 |
| --- | --- | --- |
| `neck_yaw` | `10` | 首の左右の追加回転の上限（度、0～90） |
| `neck_pitch` | `6` | 首の上下の追加回転の上限（度、0～90） |
| `head_yaw` | `25` | 頭の左右の追加回転の上限（度、0～90） |
| `head_pitch` | `15` | 頭の上下の追加回転の上限（度、0～90） |
| `arm_speed` | `90` | 肩・ひじの追従回転速度の上限（度/秒、0より大きく720以下） |

各行は `キー 値` 形式で、`#` 以降はコメントです。省略したキーには初期値を使います。
例えば `arm_speed 45` で腕をよりゆっくり動かし、`neck_pitch 3` で首の上下の動きを小さくできます。
角度を0にするとその軸の追従を止めます。首と頭の回転は親子関係で合成されます。
腕の速度制限は伸ばすとき・戻すとき・左右の切り替え時に適用します。元のモーションや物理演算の速度は変更しません。
元のモーションや頭の動きで前フレームの腕が体内に入った場合は、貫通回避を速度制限より優先します。
ファイルがない場合や不正な設定（未知のキー、重複、範囲外など）がある場合は、警告をログに出し、設定全体を初期値に戻します。

ひじはモデル基準の軸で0～135度の範囲に曲げ、肩の開きとねじれも制限します。
胴体・頭をボーン位置から推定したカプセルで保護し、腕や手が当たる目標は前方・外側へずらします。
姿勢の補間中にも判定します。これは体型を近似した回避処理であり、衣服や髪を含むメッシュとの厳密な衝突判定ではありません。

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

## スクリプト

Musubuは `src/engine/script/Musubu` にサブモジュールとして固定されています。
初回取得時は `git submodule update --init --recursive` を実行してください。
ビルドにはRustのstableツールチェーン（1.88以降）とCargoが必要です。
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
