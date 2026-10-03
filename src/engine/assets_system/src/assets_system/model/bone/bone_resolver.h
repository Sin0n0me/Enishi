#pragma once
#include "../interface_bone_resolver.h"
#include "bone_name_map.h"
#include <engine_types/assets/model/addons/bone.h>
#include <foundation/option/option.h>
#include <foundation/str/str.h>
#include <glm/glm.hpp>
#include <memory>
#include <unordered_map>
#include <vector>

namespace enishi::assets_system {
    class BoneResolver : public IBoneResolver {
      private:
        BoneNameMap name_map;

        BoneResolver(void) = delete;

      public:
        explicit BoneResolver(BoneNameMapConstructor& bone_data);

        foundation::Option<types::BoneIndex> resolve_index(
            const foundation::UTF8& bone_name) const noexcept override;
        foundation::Option<foundation::UTF8> resolve_name(
            const types::BoneIndex bone_index) const noexcept override;
        std::uint32_t bone_count(void) const noexcept override;
    };
} // namespace enishi::assets_system
