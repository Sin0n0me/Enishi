#include "bone_name_dictionary.h"
#include <algorithm>
#include <initializer_list>
#include <utility>

namespace enishi::animation {
    namespace {
        struct BoneDefinition {
            BoneRole role;
            std::string_view key;
            std::initializer_list<std::string_view> aliases;
        };
        const std::array<BoneDefinition, static_cast<std::size_t>(BoneRole::Count)> DEFINITIONS{
            {{BoneRole::Neck, "neck", {"首", "neck", "Neck"}},
                {BoneRole::Head, "head", {"頭", "head", "Head"}},
                {BoneRole::LeftUpperArm, "left_upper_arm", {"左腕", "leftUpperArm", "LeftArm"}},
                {BoneRole::LeftLowerArm,
                    "left_lower_arm",
                    {"左ひじ", "左肘", "leftLowerArm", "LeftForeArm"}},
                {BoneRole::RightUpperArm, "right_upper_arm", {"右腕", "rightUpperArm", "RightArm"}},
                {BoneRole::RightLowerArm,
                    "right_lower_arm",
                    {"右ひじ", "右肘", "rightLowerArm", "RightForeArm"}},
                {BoneRole::LeftHand, "left_hand", {"左手首", "leftHand", "LeftHand"}},
                {BoneRole::RightHand, "right_hand", {"右手首", "rightHand", "RightHand"}},
                {BoneRole::LeftEye, "left_eye", {"左目", "leftEye", "LeftEye"}},
                {BoneRole::RightEye, "right_eye", {"右目", "rightEye", "RightEye"}},
                {BoneRole::Spine, "spine", {"上半身", "spine", "Spine", "chest", "Chest"}},
                {BoneRole::Hips, "hips", {"下半身", "hips", "Hips"}},
                {BoneRole::LeftUpperLeg, "left_upper_leg", {"左足", "leftUpperLeg", "LeftUpLeg"}},
                {BoneRole::LeftLowerLeg,
                    "left_lower_leg",
                    {"左ひざ", "左膝", "leftLowerLeg", "LeftLeg"}},
                {BoneRole::LeftFoot, "left_foot", {"左足首", "leftFoot", "LeftFoot"}},
                {BoneRole::RightUpperLeg,
                    "right_upper_leg",
                    {"右足", "rightUpperLeg", "RightUpLeg"}},
                {BoneRole::RightLowerLeg,
                    "right_lower_leg",
                    {"右ひざ", "右膝", "rightLowerLeg", "RightLeg"}},
                {BoneRole::RightFoot, "right_foot", {"右足首", "rightFoot", "RightFoot"}}}};
    } // namespace
    BoneNameDictionary::BoneNameDictionary() {
        for (const auto& definition : DEFINITIONS) {
            auto& candidates = this->names[static_cast<std::size_t>(definition.role)];
            for (const auto name : definition.aliases) {
                candidates.emplace_back(name);
            }
        }
    }
    std::optional<BoneRole> BoneNameDictionary::role_from_key(std::string_view key) {
        for (const auto& definition : DEFINITIONS) {
            if (definition.key == key) {
                return definition.role;
            }
        }
        return std::nullopt;
    }
    std::string_view BoneNameDictionary::key_for_role(BoneRole role) {
        for (const auto& definition : DEFINITIONS) {
            if (definition.role == role) {
                return definition.key;
            }
        }
        return {};
    }
    bool BoneNameDictionary::set_aliases(BoneRole role, std::vector<std::string> aliases) {
        const auto index = static_cast<std::size_t>(role);
        if (!(index < this->names.size()) || aliases.empty() ||
            std::ranges::any_of(aliases, [](const auto& name) { return name.empty(); })) {
            return false;
        }
        this->names[index] = std::move(aliases);
        return true;
    }
    std::span<const std::string> BoneNameDictionary::aliases(BoneRole role) const {
        const auto index = static_cast<std::size_t>(role);
        if (!(index < this->names.size())) {
            return {};
        }
        return this->names[index];
    }
    types::BoneIndex BoneNameDictionary::find(
        std::span<const std::string> model_names, BoneRole role) const {
        // Candidate order takes precedence over the order of bones in the model.
        for (const auto& name : this->aliases(role)) {
            const auto found = std::ranges::find(model_names, name);
            if (found != model_names.end()) {
                return static_cast<types::BoneIndex>(found - model_names.begin());
            }
        }
        return types::INVALID_BONE_INDEX;
    }
} // namespace enishi::animation
