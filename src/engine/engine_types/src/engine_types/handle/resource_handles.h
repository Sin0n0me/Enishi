#pragma once
#include <engine_types/handle/handle_mapper.h>
#include <engine_types/handle/handle_type.h>
#include <engine_types/handle/renderer/render_handle.h>
#include <vector>

namespace enishi::types {
    struct ResourceHandles {
        types::HandleId resource;     // リソースそのもの
        types::HandleId binding;      // リソースを使用するために必要なパラメータ
        types::HandleId configurable; // 外部から変更可能なインターフェイス
    };
} // namespace enishi::types
