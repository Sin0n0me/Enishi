#pragma once
#include <engine_types/assets/model/morph.h>
#include <foundation/option/option.h>
#include <foundation/str/str.h>

namespace enishi::assets_system {
    class IMorphResolver {
      public:
        virtual ~IMorphResolver(void) noexcept = default;

        // 表情名からインデックスを解決する
        [[nodiscard]]
        virtual foundation::Option<types::MorphIndex> resolve_index(
            const foundation::UTF8& bone_name) const noexcept = 0;

        // インデックスから表情名を取得する
        [[nodiscard]]
        virtual foundation::Option<foundation::UTF8> resolve_name(
            const types::BoneIndex bone_index) const noexcept = 0;

        // インデックス数を返す
        [[nodiscard]]
        virtual std::uint32_t index_count(void) const noexcept = 0;
    };
} // namespace enishi::assets_system
