#include "morph_name_dictionary.h"
#include <algorithm>
#include <initializer_list>
#include <utility>

namespace enishi::animation {
    namespace {
        struct MorphDefinition {
            MorphRole role;
            std::string_view key;
            std::initializer_list<std::string_view> aliases;
        };
        const std::array<MorphDefinition, static_cast<std::size_t>(MorphRole::Count)> DEFINITIONS{
            {{MorphRole::Blink, "blink", {"まばたき", "blink", "Blink"}},
                {MorphRole::BlinkLeft,
                    "blink_left",
                    {"まばたき左", "ウィンク", "blinkLeft", "Blink_L"}},
                {MorphRole::BlinkRight,
                    "blink_right",
                    {"まばたき右", "ウィンク右", "blinkRight", "Blink_R"}},
                {MorphRole::EyeSmile, "eye_smile", {"笑い", "eyeSmile"}},
                {MorphRole::MouthA, "mouth_a", {"あ", "aa", "A"}},
                {MorphRole::MouthI, "mouth_i", {"い", "ih", "I"}},
                {MorphRole::MouthU, "mouth_u", {"う", "ou", "U"}},
                {MorphRole::MouthE, "mouth_e", {"え", "ee", "E"}},
                {MorphRole::MouthO, "mouth_o", {"お", "oh", "O"}},
                {MorphRole::MouthSmile, "mouth_smile", {"にこり", "mouthSmile"}},
                {MorphRole::BrowUp, "brow_up", {"上", "browUp"}},
                {MorphRole::BrowDown, "brow_down", {"下", "browDown"}},
                {MorphRole::BrowAngry, "brow_angry", {"怒り", "browAngry"}},
                {MorphRole::BrowSad, "brow_sad", {"困る", "browSad"}}}};
    } // namespace
    MorphNameDictionary::MorphNameDictionary() {
        for (const auto& definition : DEFINITIONS) {
            auto& candidates = this->names[static_cast<std::size_t>(definition.role)];
            for (const auto name : definition.aliases) {
                candidates.emplace_back(name);
            }
        }
    }
    std::optional<MorphRole> MorphNameDictionary::role_from_key(std::string_view key) {
        for (const auto& definition : DEFINITIONS) {
            if (definition.key == key) {
                return definition.role;
            }
        }
        return std::nullopt;
    }
    std::string_view MorphNameDictionary::key_for_role(MorphRole role) {
        for (const auto& definition : DEFINITIONS) {
            if (definition.role == role) {
                return definition.key;
            }
        }
        return {};
    }
    bool MorphNameDictionary::set_aliases(MorphRole role, std::vector<std::string> aliases) {
        const auto index = static_cast<std::size_t>(role);
        if (!(index < this->names.size()) || aliases.empty() ||
            std::ranges::any_of(aliases, [](const auto& name) { return name.empty(); })) {
            return false;
        }
        this->names[index] = std::move(aliases);
        return true;
    }
    std::span<const std::string> MorphNameDictionary::aliases(MorphRole role) const {
        const auto index = static_cast<std::size_t>(role);
        if (!(index < this->names.size())) {
            return {};
        }
        return this->names[index];
    }
    types::MorphIndex MorphNameDictionary::find(
        std::span<const std::string> model_names, MorphRole role) const {
        // Candidate order takes precedence over the order of morphs in the model.
        for (const auto& name : this->aliases(role)) {
            const auto found = std::ranges::find(model_names, name);
            if (found != model_names.end()) {
                return static_cast<types::MorphIndex>(found - model_names.begin());
            }
        }
        return types::INVALID_MORPH_INDEX;
    }
    types::MorphIndex MorphNameDictionary::find(
        std::span<const types::MorphTarget> targets, MorphRole role) const {
        for (const auto& name : this->aliases(role)) {
            const auto found = std::ranges::find(targets, name, &types::MorphTarget::name);
            if (found != targets.end()) {
                return static_cast<types::MorphIndex>(found - targets.begin());
            }
        }
        return types::INVALID_MORPH_INDEX;
    }
} // namespace enishi::animation
