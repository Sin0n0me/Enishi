#pragma once
#include <array>
#include <engine_types/assets/model/addons/bone.h>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace enishi::animation {
    enum class BoneRole {
        Neck,
        Head,
        LeftUpperArm,
        LeftLowerArm,
        RightUpperArm,
        RightLowerArm,
        LeftHand,
        RightHand,
        LeftEye,
        RightEye,
        Spine,
        Hips,
        LeftUpperLeg,
        LeftLowerLeg,
        LeftFoot,
        RightUpperLeg,
        RightLowerLeg,
        RightFoot,
        Count
    };

    class BoneNameDictionary {
      private:
        static constexpr std::size_t ROLE_COUNT = static_cast<std::size_t>(BoneRole::Count);
        std::array<std::vector<std::string>, ROLE_COUNT> names;

      public:
        BoneNameDictionary();
        [[nodiscard]] static std::optional<BoneRole> role_from_key(std::string_view key);
        [[nodiscard]] static std::string_view key_for_role(BoneRole role);
        [[nodiscard]] bool set_aliases(BoneRole role, std::vector<std::string> aliases);
        [[nodiscard]] std::span<const std::string> aliases(BoneRole role) const;
        [[nodiscard]] types::BoneIndex find(
            std::span<const std::string> model_names, BoneRole role) const;
    };
} // namespace enishi::animation
