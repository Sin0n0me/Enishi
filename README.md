# Enishi

## ビルド

プロジェクトルートの `build\build.cmd` で、レンダリングバックエンドとツールチェーンを指定してビルドできます。

```bat
build\build.cmd <opengl|directx> [Debug|Release|RelWithDebInfo|MinSizeRel] [visualstudio|clang]
```

構成を省略した場合は `Debug`、ツールチェーンを省略した場合は`visualstudio` を使用します。

### Visual Studio

```bat
build\build.cmd opengl Debug
build\build.cmd directx Release
```

生成先はそれぞれ `build\opengl\visualstudio` と`build\directx\visualstudio` です。


Visual Studio ビルドでは、Visual Studio 18 2026 を優先して使用します。
18 が見つからない場合は Visual Studio 17 2022 を使用します。

### Clang + Ninja

MSBuild を使わないビルドには Clang と Ninja を使用します。`clang` と`ninja` の両方が `PATH` から実行できる状態にしてください。

```bat
build\build.cmd opengl Debug clang
build\build.cmd directx Release clang
```

生成先はそれぞれ `build\opengl\clang` と `build\directx\clang` です。


