#include "vrm_expression.h"
#include "vrm_expression_uv.h"
#include <algorithm>
#include <cmath>
#include <engine_types/renderer/uniform_buffer/material.h>
#include <set>

namespace enishi::assets_system {
    namespace {
        std::string expression_name(std::string name) {
            static const std::map<std::string, std::string> aliases{{"a", "aa"},
                {"i", "ih"},
                {"u", "ou"},
                {"e", "ee"},
                {"o", "oh"},
                {"joy", "happy"},
                {"sorrow", "sad"},
                {"fun", "relaxed"},
                {"blink_l", "blinkLeft"},
                {"blink_r", "blinkRight"},
                {"lookup", "lookUp"},
                {"lookdown", "lookDown"},
                {"lookleft", "lookLeft"},
                {"lookright", "lookRight"}};
            const auto found = aliases.find(name);
            return found != aliases.end() ? found->second : name;
        }

        types::ExpressionOverride override_mode(const VRMJson& value) {
            if (value.value == "block") {
                return types::ExpressionOverride::Block;
            }
            return value.value == "blend" ? types::ExpressionOverride::Blend
                                          : types::ExpressionOverride::None;
        }

        bool bind_morphs(
            VRMContext& context, types::MorphTarget& target, const VRMJson& expression) {
            const auto& binds = expression[context.legacy ? "binds" : "morphTargetBinds"];
            for (const auto& bind : binds.elements) {
                // VRM 0 stores percent weights; VRM 1 stores unit weights.
                constexpr float PERCENT = 100;
                const auto weight = bind["weight"].number() / (context.legacy ? PERCENT : 1.0f);
                if (!std::isfinite(weight) || weight < 0 || weight > 1) {
                    return context.fail("Invalid VRM expression weight");
                }
                bool found = false;
                for (const auto& [key, index] : context.morphs) {
                    const auto* mesh = context.source.nodes[key.first].mesh;
                    const bool matches =
                        context.legacy ? static_cast<std::size_t>(mesh - context.source.meshes) ==
                                             bind["mesh"].index()
                                       : key.first == bind["node"].index();
                    if (matches && key.second == bind["index"].index()) {
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
            const auto& binds =
                expression[context.legacy ? "materialValues" : "materialColorBinds"];
            for (const auto& bind : binds.elements) {
                const auto property = bind[context.legacy ? "propertyName" : "type"].value;
                const auto& values = bind["targetValue"].elements;
                if (context.legacy && property == "_MainTex_ST") {
                    continue;
                }
                std::string destination;
                if (property == "color" || property == "_Color") {
                    destination = "diffuse";
                } else if (property == "outlineColor" || property == "_OutlineColor") {
                    destination = "outline_color";
                } else if (property == "shadeColor" || property == "_ShadeColor") {
                    destination = "ambient";
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
                            ? source.name != nullptr && bind["materialName"].value == source.name
                            : bind["material"].index() == source_index;
                    if (!matches) {
                        continue;
                    }
                    found = true;
                    const auto base = types::make_uniform_material(context.model.materials[index]);
                    glm::vec4 initial = base.diffuse;
                    if (destination == "outline_color") {
                        initial = base.outline_color;
                    } else if (destination == "ambient") {
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
            target.weight_mode = source["isBinary"].boolean() ? types::MorphWeightMode::Binary
                                                              : types::MorphWeightMode::Continuous;
            target.override_blink = override_mode(source["overrideBlink"]);
            target.override_mouth = override_mode(source["overrideMouth"]);
            target.override_look_at = override_mode(source["overrideLookAt"]);
            if (preset && (target.name == "blink" || target.name == "blinkLeft" ||
                              target.name == "blinkRight")) {
                target.category = types::ExpressionCategory::Blink;
            } else if (preset && (target.name == "lookUp" || target.name == "lookDown" ||
                                     target.name == "lookLeft" || target.name == "lookRight")) {
                target.category = types::ExpressionCategory::LookAt;
            } else if (preset &&
                       (target.name == "aa" || target.name == "ih" || target.name == "ou" ||
                           target.name == "ee" || target.name == "oh")) {
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
            for (const auto& expression :
                context.extension["blendShapeMaster"]["blendShapeGroups"].elements) {
                const auto& preset = expression["presetName"].value;
                const auto name =
                    preset.empty() || preset == "unknown" ? expression["name"].value : preset;
                if (!add_expression(
                        context, name, expression, !preset.empty() && preset != "unknown")) {
                    return false;
                }
            }
        } else {
            for (const auto category : {"preset", "custom"}) {
                for (const auto& [name, expression] :
                    context.extension["expressions"][category].members) {
                    if (!add_expression(
                            context, name, expression, std::string_view(category) == "preset")) {
                        return false;
                    }
                }
            }
        }
        return true;
    }
} // namespace enishi::assets_system
