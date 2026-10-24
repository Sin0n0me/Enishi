#pragma once
#include <array>
#include <engine_types/assets/model/addons/morph.h>
#include <engine_types/assets/model/addons/morph_target.h>
#include <foundation/option/option.h>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace enishi::animation {
    enum class MorphRole {
        Blink,
        BlinkLeft,
        BlinkRight,
        EyeSmile,
        MouthA,
        MouthI,
        MouthU,
        MouthE,
        MouthO,
        MouthSmile,
        BrowUp,
        BrowDown,
        BrowAngry,
        BrowSad,
        Count
    };

    class MorphNameDictionary {
      private:
        static constexpr std::size_t ROLE_COUNT = static_cast<std::size_t>(MorphRole::Count);
        std::array<std::vector<std::string>, ROLE_COUNT> names;

      public:
        MorphNameDictionary();
        [[nodiscard]] static foundation::Option<MorphRole> role_from_key(std::string_view key);
        [[nodiscard]] static std::string_view key_for_role(MorphRole role);
        [[nodiscard]] bool set_aliases(MorphRole role, std::vector<std::string> aliases);
        [[nodiscard]] std::span<const std::string> aliases(MorphRole role) const;
        [[nodiscard]] types::MorphIndex find(
            std::span<const std::string> model_names, MorphRole role) const;
        [[nodiscard]] types::MorphIndex find(
            std::span<const types::MorphTarget> targets, MorphRole role) const;
    };
} // namespace enishi::animation
