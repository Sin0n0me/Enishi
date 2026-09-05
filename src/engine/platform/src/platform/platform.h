#pragma once

/**
 * この層の目的
 *
 * エンジンのコアより下のレイヤー
 * この層ではより低いレイヤーの各プラットフォームに依存した情報を隠蔽し,
 * 上のレイヤーは整数型のハンドルをもとにプラットフォームに依存しない抽象的な操作を扱えるようにする
 */

namespace enishi::platform {} // namespace enishi::platform