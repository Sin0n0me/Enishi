#pragma once
#include <foundation/result/result.h>
#include <string>

namespace enishi::assets_system {
    enum class IOError {
        FileNotFound,
        PermissionDenied,
        InvalidFormat,
        ReadFailed,
        UnexpectedEof,
        InvalidData,
        InvalidStreamPosition,
        BrokenStream,
        MismatchHeader,
    };

    enum class AssetError {
        AlreadyHasAsset,    //
        NotFound,           //
        IOError,            //
        InvalidAssetData,   //
        NotLoaded,          // 読み込まれていない
        UnsupportedVersion, // 非対応のバージョン
        UnknownFormat,      // 未知のフォーマット
        MissingDependency,  // 依存アセットの不足
        OutOfMemory,        // メモリ不足
        AccessDenied,       // 権限エラー
    };

    template <typename T> using IOResult = foundation::Result<T, IOError>;
} // namespace enishi::assets_system