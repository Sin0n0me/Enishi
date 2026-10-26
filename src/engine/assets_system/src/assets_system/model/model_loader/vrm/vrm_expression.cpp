#include "vrm_expression.h"
#include "vrm_expression_uv.h"
#include "vrm_schema.h"
#include <algorithm>
#include <cmath>
#include <engine_types/renderer/uniform_buffer/material.h>
#include <set>

namespace enishi::assets_system {
    namespace {
        std::string expression_name(std::string name) {
            static const std::map<std::string, std::string> aliases{
                {vrm_schema::legacy_preset::A, vrm_schema::preset::AA},
                {vrm_schema::legacy_preset::I, vrm_schema::preset::IH},
                {vrm_schema::legacy_preset::U, vrm_schema::preset::OU},
                {vrm_schema::legacy_preset::E, vrm_schema::preset::EE},
                {vrm_schema::legacy_preset::O, vrm_schema::preset::OH},
                {vrm_schema::legacy_preset::JOY, vrm_schema::preset::HAPPY},
                {vrm_schema::legacy_preset::SORROW, vrm_schema::preset::SAD},
                {vrm_schema::legacy_preset::FUN, vrm_schema::preset::RELAXED},
                {vrm_schema::legacy_preset::BLINK_L, vrm_schema::preset::BLINK_LEFT},
                {vrm_schema::legacy_preset::BLINK_R, vrm_schema::preset::BLINK_RIGHT},
                {vrm_schema::legacy_preset::LOOKUP, vrm_schema::preset::LOOK_UP},
                {vrm_schema::legacy_preset::LOOKDOWN, vrm_schema::preset::LOOK_DOWN},
                {vrm_schema::legacy_preset::LOOKLEFT, vrm_schema::preset::LOOK_LEFT},
                {vrm_schema::legacy_preset::LOOKRIGHT, vrm_schema::preset::LOOK_RIGHT}};
            const auto found = aliases.find(name);
            return found != aliases.end() ? found->second : name;
        }

        types::ExpressionOverride override_mode(const VRMJson& value) {
            if (value.value == vrm_schema::override_mode::BLOCK) {
                return types::ExpressionOverride::Block;
            }
            return value.value == vrm_schema::override_mode::BLEND
                       ? types::ExpressionOverride::Blend
                       : types::ExpressionOverride::None;
        }

        bool bind_morphs(
            VRMContext& context, types::MorphTarget& target, const VRMJson& expression) {
            const auto& binds = expression[context.legacy ? vrm_schema::legacy_key::BINDS
                                                          : vrm_schema::key::MORPH_TARGET_BINDS];
            for (const auto& bind : binds.elements) {
                // VRM 0 stores percent weights; VRM 1 stores unit weights.
                constexpr float PERCENT = 100;
                const auto weight =
                    bind[vrm_schema::key::WEIGHT].number() / (context.legacy ? PERCENT : 1.0f);
                if (!std::isfinite(weight) || weight < 0 || weight > 1) {
                    return context.fail("Invalid VRM expression weight");
                }
                bool found = false;
                for (const auto& [key, index] : context.morphs) {
                    const auto* mesh = context.source.nodes[key.first].mesh;
                    const bool matches =
                        context.legacy ? static_cast<std::size_t>(mesh - context.source.meshes) ==
                                             bind[vrm_schema::key::MESH].index()
                                       : key.first == bind[vrm_schema::key::NODE].index();
                    if (matches && key.second == bind[vrm_schema::key::INDEX].index()) {
                        target.offsets.emplace_back(types::MorphWeightOffset{index, weight});
                        found = true;
                    }
                }
                if (!found) {
                    return context.fail("VRM expression references a missing morph");
                }
            }
            return true;
        }

        bool bind_colors(
            VRMContext& context, types::MorphTarget& target, const VRMJson& expression) {
            const auto& binds = expression[context.legacy ? vrm_schema::legacy_key::MATERIAL_VALUES
                                                          : vrm_schema::key::MATERIAL_COLOR_BINDS];
            for (const auto& bind : binds.elements) {
                const auto property = bind[context.legacy ? vrm_schema::legacy_key::PROPERTY_NAME
                                                          : vrm_schema::key::TYPE]
                                          .value;
                const auto& values = bind[vrm_schema::key::TARGET_VALUE].elements;
                if (context.legacy &&
                    property == vrm_schema::legacy_material_property::MAIN_TEXTURE_TRANSFORM) {
                    continue;
                }
                std::string destination;
                if (property == vrm_schema::material_property::COLOR ||
                    property == vrm_schema::legacy_material_property::COLOR) {
                    destination = vrm_schema::material_target::DIFFUSE;
                } else if (property == vrm_schema::material_property::OUTLINE_COLOR ||
                           property == vrm_schema::legacy_material_property::OUTLINE_COLOR) {
                    destination = vrm_schema::material_target::OUTLINE_COLOR;
                } else if (property == vrm_schema::material_property::SHADE_COLOR ||
                           property == vrm_schema::legacy_material_property::SHADE_COLOR) {
                    destination = vrm_schema::material_target::AMBIENT;
                } else {
                    return context.fail(
                        "Unsupported VRM expression material property: " + property);
                }
                if (values.size() != 4) {
                    return context.fail("Invalid VRM expression color");
                }
                glm::vec4 value;
                for (glm::length_t component = 0; component < 4; ++component) {
                    value[component] = values[component].number();
                    if (!std::isfinite(value[component])) {
                        return context.fail("Non-finite VRM expression color");
                    }
                }
                bool found = false;
                for (std::size_t index = 0; index < context.material_sources.size(); ++index) {
                    const auto source_index = context.material_sources[index];
                    if (!(source_index < context.source.materials_count)) {
                        continue;
                    }
                    const auto& source = context.source.materials[source_index];
                    const bool matches =
                        context.legacy
                            ? source.name != nullptr &&
                                  bind[vrm_schema::legacy_key::MATERIAL_NAME].value == source.name
                            : bind[vrm_schema::key::MATERIAL].index() == source_index;
                    if (!matches) {
                        continue;
                    }
                    found = true;
                    const auto base = types::make_uniform_material(context.model.materials[index]);
                    glm::vec4 initial = base.diffuse;
                    if (destination == vrm_schema::material_target::OUTLINE_COLOR) {
                        initial = base.outline_color;
                    } else if (destination == vrm_schema::material_target::AMBIENT) {
                        initial = glm::vec4(base.ambient, 1);
                    }
                    // VRM color = base + expression_weight * (target - base).
                    target.offsets.emplace_back(
                        types::MaterialMorphOffset{static_cast<std::uint32_t>(index),
                            types::MorphOperation::Add,
                            {{destination, value - initial}}});
                }
                if (!found) {
                    return context.fail("VRM expression references a missing material");
                }
            }
            return true;
        }

        bool add_expression(
            VRMContext& context, std::string name, const VRMJson& source, bool preset) {
            types::MorphTarget target;
            target.name =
                context.legacy && preset ? expression_name(std::move(name)) : std::move(name);
            if (target.name.empty() ||
                std::ranges::any_of(context.targets.targets,
                    [&](const auto& existing) { return existing.name == target.name; })) {
                return context.fail("Empty or duplicate VRM expression name");
            }
            target.expression = true;
            target.weight_mode = source[vrm_schema::key::IS_BINARY].boolean()
                                     ? types::MorphWeightMode::Binary
                                     : types::MorphWeightMode::Continuous;
            target.override_blink = override_mode(source[vrm_schema::key::OVERRIDE_BLINK]);
            target.override_mouth = override_mode(source[vrm_schema::key::OVERRIDE_MOUTH]);
            target.override_look_at = override_mode(source[vrm_schema::key::OVERRIDE_LOOK_AT]);
            if (preset && (target.name == vrm_schema::preset::BLINK ||
                              target.name == vrm_schema::preset::BLINK_LEFT ||
                              target.name == vrm_schema::preset::BLINK_RIGHT)) {
                target.category = types::ExpressionCategory::Blink;
            } else if (preset && (target.name == vrm_schema::preset::LOOK_UP ||
                                     target.name == vrm_schema::preset::LOOK_DOWN ||
                                     target.name == vrm_schema::preset::LOOK_LEFT ||
                                     target.name == vrm_schema::preset::LOOK_RIGHT)) {
                target.category = types::ExpressionCategory::LookAt;
            } else if (preset && (target.name == vrm_schema::preset::AA ||
                                     target.name == vrm_schema::preset::IH ||
                                     target.name == vrm_schema::preset::OU ||
                                     target.name == vrm_schema::preset::EE ||
                                     target.name == vrm_schema::preset::OH)) {
                target.category = types::ExpressionCategory::Mouth;
            }
            if (!bind_morphs(context, target, source) || !bind_colors(context, target, source)) {
                return false;
            }
            if (!bind_vrm_texture_transforms(context, target, source)) {
                return false;
            }
            context.targets.targets.push_back(std::move(target));
            return true;
        }
    } // namespace

    bool make_vrm_expressions(VRMContext& context) {
        if (context.legacy) {
            for (const auto& expression : context
                                              .extension[vrm_schema::legacy_key::BLEND_SHAPE_MASTER]
                                                        [vrm_schema::legacy_key::BLEND_SHAPE_GROUPS]
                                              .elements) {
                const auto& preset = expression[vrm_schema::legacy_key::PRESET_NAME].value;
                const auto name = preset.empty() || preset == vrm_schema::legacy_preset::UNKNOWN
                                      ? expression[vrm_schema::key::NAME].value
                                      : preset;
                if (!add_expression(context,
                        name,
                        expression,
                        !preset.empty() && preset != vrm_schema::legacy_preset::UNKNOWN)) {
                    return false;
                }
            }
        } else {
            for (const auto category : {vrm_schema::key::PRESET, vrm_schema::key::CUSTOM}) {
                for (const auto& [name, expression] :
                    context.extension[vrm_schema::key::EXPRESSIONS][category].members) {
                    if (!add_expression(context,
                            name,
                            expression,
                            std::string_view(category) == vrm_schema::key::PRESET)) {
                        return false;
                    }
                }
            }
        }
        return true;
    }
} // namespace enishi::assets_system
