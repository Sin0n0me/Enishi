#pragma once
#include <engine_types/assets/model/addons/bone.h>
#include <foundation/option/option.h>
#include <foundation/str/str.h>

namespace enishi::assets_system {
    class IBoneResolver {
      public:
        virtual ~IBoneResolver(void) noexcept = default;

        // 名前からインデックスを解決する
        [[nodiscard]]
        virtual foundation::Option<types::BoneIndex> resolve_index(
            const foundation::UTF8& bone_name) const noexcept = 0;

        // インデックスから名前を取得する
        [[nodiscard]]
        virtual foundation::Option<foundation::UTF8> resolve_name(
            const types::BoneIndex bone_index) const noexcept = 0;

        // ボーン数を返す
        [[nodiscard]]
        virtual std::uint32_t bone_count(void) const noexcept = 0;
    };
} // namespace enishi::assets_system
