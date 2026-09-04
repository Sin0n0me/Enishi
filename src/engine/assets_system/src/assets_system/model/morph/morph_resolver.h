#pragma once
#include "../interface_morph_resolver.h"
#include "morph_name_map.h"
#include <engine_types/assets/model/addons/bone.h>
#include <foundation/option/option.h>
#include <foundation/str/str.h>
#include <glm/glm.hpp>
#include <memory>
#include <unordered_map>
#include <vector>

namespace enishi::assets_system {
    class MorphResolver : public IMorphResolver {
      private:
        MorphNameMap name_map;

        MorphResolver(void) = delete;

      public:
        explicit MorphResolver(MorphNameMapConstructor& morph_data);

        foundation::Option<types::MorphIndex> resolve_index(
            const foundation::UTF8& morph_name) const noexcept override;
        foundation::Option<foundation::UTF8> resolve_name(
            const types::BoneIndex morph_index) const noexcept override;
        std::uint32_t index_count(void) const noexcept override;
    };
} // namespace enishi::assets_system
