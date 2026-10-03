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
