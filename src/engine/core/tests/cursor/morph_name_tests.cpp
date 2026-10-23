#include "morph_name_tests.h"
#include <core/system/animation/animation_system.h>
#include <core/system/animation/morph_name_config.h>
#include <sstream>
#include <stdexcept>

using namespace enishi;
namespace {
    void require(bool value, const char* message) {
        if (!value) {
            throw std::runtime_error(message);
        }
    }
    void lookup_tests() {
        animation::MorphNameDictionary dictionary;
        using Role = animation::MorphRole;
        for (int index = 0; index < static_cast<int>(Role::Count); ++index) {
            const auto role = static_cast<Role>(index);
            require(animation::MorphNameDictionary::role_from_key(
                        animation::MorphNameDictionary::key_for_role(role)) == role,
                "Every morph role must round-trip through its key");
            const std::vector<std::string> names{dictionary.aliases(role).front()};
            require(dictionary.find(names, role) == 0, "Every role has searchable defaults");
        }
        const std::vector<std::string> names{"fallback", "preferred", "blink"};
        require(dictionary.set_aliases(Role::Blink, {"preferred", "fallback"}), "Override aliases");
        require(dictionary.find(names, Role::Blink) == 1, "Alias priority beats model ordering");
        require(!dictionary.set_aliases(Role::Blink, {}) &&
                    !dictionary.set_aliases(Role::Blink, {""}) &&
                    !dictionary.set_aliases(Role::Count, {"invalid"}),
            "Reject invalid aliases");
        require(dictionary.find(names, Role::Blink) == 1, "Invalid replacement preserves aliases");
        require(dictionary.find(names, Role::MouthA) == types::INVALID_MORPH_INDEX &&
                    dictionary.find(names, Role::Count) == types::INVALID_MORPH_INDEX,
            "Missing or invalid roles cannot select another morph");
        require(dictionary.aliases(static_cast<Role>(-1)).empty(), "Invalid enum is safe");
    }
    void config_tests() {
        std::istringstream input("\xEF\xBB\xBF# header\nversion = 1\n[roles.blink]\naliases = "
                                 "[\n\"custom blink\", 'fallback',\n] # tail");
        const auto parsed = core::parse_morph_name_config(input);
        require(parsed.is_ok(), "BOM, quoted spaces and comments are supported");
        const std::vector<std::string> names{"fallback", "custom blink", "aa", "blink"};
        require(parsed.unwrap().find(names, animation::MorphRole::Blink) == 1 &&
                    parsed.unwrap().find(names, animation::MorphRole::MouthA) == 2,
            "Only configured roles replace defaults");
        const std::vector<std::string> old_names{"blink"};
        require(parsed.unwrap().find(old_names, animation::MorphRole::Blink) ==
                    types::INVALID_MORPH_INDEX,
            "Configured aliases replace rather than append to defaults");
        for (const auto* text : {"",
                 "version = 2",
                 "version = 1.0",
                 "version = true",
                 "version = 1\nunknown = 1",
                 "version = 1\nroles = []",
                 "version = 1\n[roles.unknown]\naliases = ['name']",
                 "version = 1\n[roles.blink]",
                 "version = 1\n[roles.blink]\naliases = []",
                 "version = 1\n[roles.blink]\naliases = ['']",
                 "version = 1\n[roles.blink]\naliases = [1]",
                 "version = 1\n[roles.blink]\naliases = 'name'",
                 "version = 1\n[roles.blink]\naliases = ['a']\nextra = 1",
                 "version = 1\n[roles.blink]\naliases = ['a']\naliases = ['b']",
                 "version = 1\n[roles.blink]\naliases = ['a']\n[roles.blink]",
                 "version = 1\n[roles.blink]\naliases = ['unterminated"}) {
            std::istringstream invalid(text);
            require(
                core::parse_morph_name_config(invalid).is_err(), "Reject malformed configuration");
        }
        std::istringstream defaults("version = 1");
        require(core::parse_morph_name_config(defaults).is_ok(), "Allow default-only dictionary");
        std::istringstream failed;
        failed.setstate(std::ios::badbit);
        require(core::parse_morph_name_config(failed).is_err(), "Propagate stream failure");
    }
    void system_tests() {
        auto registry = std::make_shared<ecs::Registry>();
        core::AnimationSystem system(registry);
        const auto entity = registry->create();
        require(
            system.find_morph(entity, animation::MorphRole::Blink) == types::INVALID_MORPH_INDEX,
            "Entity without a model has no morph");
        component::ModelComponent model;
        model.morph_targets.targets = {{.name = "custom blink"}, {.name = "blink"}};
        require(registry->emplace<component::ModelComponent>(entity, model).is_ok(),
            "Create synthetic model");
        require(
            system.find_morph(entity, animation::MorphRole::Blink) == 1, "Use default dictionary");
        std::istringstream input("version = 1\n[roles.blink]\naliases = [\"custom blink\"]");
        const auto parsed = core::parse_morph_name_config(input);
        require(parsed.is_ok(), "Parse override");
        system.set_morph_names(parsed.unwrap());
        require(system.find_morph(entity, animation::MorphRole::Blink) == 0,
            "Apply configured dictionary to model targets");
    }
} // namespace
void morph_name_tests() {
    lookup_tests();
    config_tests();
    system_tests();
}
