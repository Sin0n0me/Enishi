# Enishi

## ビルド

プロジェクトルートの `build\build.cmd` で、レンダリングバックエンドとツールチェーンを指定してビルドできます。

`build\build.cmd` は生成ファイルです。変更する場合は
`build\generate_build.py` を編集してから、次のコマンドで再生成してください。

```bat
python build\generate_build.py
```

```bat
build\build.cmd <opengl|directx> [Debug|Release|RelWithDebInfo|MinSizeRel] [auto|visualstudio|clang|gcc]
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

例えば、用意したモーションを結月ゆかりのどの部位のクリックでも1回再生する設定は次のとおりです。
以下は設定例であり、初期設定には含まれません。

```text
"結月ゆかり" "click" "*" motion "../assets/motion/手を振る.vmd" once
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
