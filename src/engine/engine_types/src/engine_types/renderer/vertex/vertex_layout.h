#pragma once
#include <cstdint>
#include <vector>

namespace enishi::types {
    // 1要素のフォーマット
    enum class VertexFormat : std::uint8_t {
        Float32x1, // f32 x1
        Float32x2, // f32 x2
        Float32x3, // f32 x3
        Float32x4, // f32 x4
        UInt16x1,  // u16 x1
        UInt16x2,  // u16 x2
        UInt16x4,  // u16 x4
        Int16x1,   // i16 x1
        Int16x2,   // i16 x2
        Int16x4,   // i16 x4
        UInt32x1,  // u32 x1
        UInt32x2,  // u32 x2
        UInt32x3,  // u32 x3
        UInt32x4,  // u32 x4
        Int32x1,   // i32 x1
        Int32x2,   // i32 x2
        Int32x3,   // i32 x3
        Int32x4,   // i32 x4
    };

    // 1頂点属性の定義
    // DirectX の D3D11_INPUT_ELEMENT_DESC 1要素に相当
    struct VertexAttribute {
        VertexFormat format;
        std::uint32_t offset;  // 頂点構造体先頭からのバイトオフセット
        std::uint32_t binding; // バインディング番号（複数VBOの場合）
    };

    // 頂点バッファ1本分のストライド定義
    struct VertexBinding {
        std::uint32_t binding; // バインディング番号
        std::uint32_t stride;  // 1頂点のバイト数
    };

    // 頂点レイアウト全体
    // DirectX の InputLayout 全体に相当
    struct VertexLayout {
        std::vector<VertexBinding> bindings;
        std::vector<VertexAttribute> attributes;

        // フォーマットのバイトサイズを返すヘルパー
        [[nodiscard]]
        static constexpr std::uint32_t format_size(const VertexFormat fmt) noexcept {
            switch (fmt) {
                case VertexFormat::Int16x1:
                case VertexFormat::UInt16x1:
                    return 2;
                case VertexFormat::Int16x2:
                case VertexFormat::UInt16x2:
                    return 4;
                case VertexFormat::Int16x4:
                case VertexFormat::UInt16x4:
                    return 8;
                case VertexFormat::Float32x1:
                case VertexFormat::Int32x1:
                case VertexFormat::UInt32x1:
                    return 4;
                case VertexFormat::Float32x2:
                case VertexFormat::Int32x2:
                case VertexFormat::UInt32x2:
                    return 8;
                case VertexFormat::Float32x3:
                case VertexFormat::Int32x3:
                case VertexFormat::UInt32x3:
                    return 12;
                case VertexFormat::Float32x4:
                case VertexFormat::Int32x4:
                case VertexFormat::UInt32x4:
                    return 16;
                default:
                    return 0;
            }
        }
    };
} // namespace enishi::types