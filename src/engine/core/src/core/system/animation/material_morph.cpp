#include "material_morph.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <string_view>

namespace enishi::core {
    namespace {
        struct Property {
            std::string_view name;
            std::size_t offset;
            std::size_t width;
            std::size_t addition;
        };
        constexpr std::size_t NO_ADDITION = sizeof(types::UniformMaterial);
        constexpr std::array PROPERTIES{
            Property{"diffuse",
                offsetof(types::UniformMaterial, diffuse),
                sizeof(glm::vec4),
                NO_ADDITION},
            Property{"specular",
                offsetof(types::UniformMaterial, specular),
                sizeof(glm::vec3),
                NO_ADDITION},
            Property{"shininess",
                offsetof(types::UniformMaterial, shininess),
                sizeof(float),
                NO_ADDITION},
            Property{"ambient",
                offsetof(types::UniformMaterial, ambient),
                sizeof(glm::vec3),
                NO_ADDITION},
            Property{"outline_color",
                offsetof(types::UniformMaterial, outline_color),
                sizeof(glm::vec4),
                NO_ADDITION},
            Property{"outline_width",
                offsetof(types::UniformMaterial, outline_parameters),
                sizeof(float),
                NO_ADDITION},
            Property{"base_color_texture_tint",
                offsetof(types::UniformMaterial, base_color_texture_factor),
                sizeof(glm::vec4),
                offsetof(types::UniformMaterial, base_color_texture_add)},
            Property{"environment_texture_tint",
                offsetof(types::UniformMaterial, environment_texture_factor),
                sizeof(glm::vec4),
                offsetof(types::UniformMaterial, environment_texture_add)},
            Property{"shading_ramp_texture_tint",
                offsetof(types::UniformMaterial, shading_ramp_texture_factor),
                sizeof(glm::vec4),
                offsetof(types::UniformMaterial, shading_ramp_texture_add)},
        };
        using Factors = std::array<glm::vec4, PROPERTIES.size()>;

        bool finite(glm::vec4 value) {
            for (glm::length_t component = 0; component < value.length(); ++component) {
                if (!std::isfinite(value[component])) {
                    return false;
                }
            }
            return true;
        }

        foundation::Result<void, MorphError> accumulate(const types::MaterialMorphOffset& offset,
            float weight,
            std::vector<Factors>& multiply,
            std::vector<Factors>& add) {
            constexpr std::uint32_t ALL_MATERIALS = UINT32_MAX;
            const auto all = offset.material == ALL_MATERIALS;
            if (!all && !(offset.material < multiply.size())) {
                return foundation::Error(
                    MorphError::InvalidReference, "Material morph index is out of range");
            }
            const auto begin = all ? 0 : static_cast<std::size_t>(offset.material);
            const auto end = all ? multiply.size() : begin + 1;
            for (const auto& property : offset.properties) {
                const auto found = std::find_if(PROPERTIES.begin(),
                    PROPERTIES.end(),
                    [&](const auto& entry) { return entry.name == property.property; });
                if (found == PROPERTIES.end()) {
                    return foundation::Error(MorphError::UnsupportedOffset,
                        "Unknown material morph property: " + property.property);
                }
                const auto index = static_cast<std::size_t>(found - PROPERTIES.begin());
                for (std::size_t material = begin; material < end; ++material) {
                    if (offset.operation == types::MorphOperation::Multiply) {
                        multiply[material][index] *=
                            glm::vec4(1) + (property.value - glm::vec4(1)) * weight;
                    } else {
                        add[material][index] += property.value * weight;
                    }
                    if (!finite(multiply[material][index]) || !finite(add[material][index])) {
                        return foundation::Error(
                            MorphError::InvalidWeight, "Non-finite material morph factor");
                    }
                }
            }
            return {};
        }

        bool apply_factors(
            types::UniformMaterial& material, const Factors& multiply, const Factors& add) {
            auto* bytes = reinterpret_cast<std::byte*>(&material);
            for (std::size_t index = 0; index < PROPERTIES.size(); ++index) {
                const auto& property = PROPERTIES[index];
                glm::vec4 value(0);
                std::memcpy(&value, bytes + property.offset, property.width);
                value *= multiply[index];
                if (property.addition == NO_ADDITION) {
                    value += add[index];
                } else {
                    std::memcpy(bytes + property.addition, &add[index], property.width);
                }
                if (!finite(value)) {
                    return false;
                }
                std::memcpy(bytes + property.offset, &value, property.width);
            }
            return true;
        }
    } // namespace

    foundation::Result<std::vector<types::UniformMaterial>, MorphError> evaluate_material_morphs(
        std::span<const types::Material> materials,
        std::span<const types::MorphTarget> targets,
        std::span<const float> weights) {
        const auto effective = evaluate_morph_weights(targets, weights);
        if (effective.is_err()) {
            return effective.propagation(MorphError::InvalidWeight);
        }
        std::vector<Factors> multiply(materials.size());
        std::vector<Factors> add(materials.size());
        for (auto& factors : multiply) {
            factors.fill(glm::vec4(1));
        }
        for (auto& factors : add) {
            factors.fill(glm::vec4(0));
        }
        for (std::size_t index = 0; index < targets.size(); ++index) {
            if (effective.unwrap()[index] == 0.0f) {
                continue;
            }
            for (const auto& offset : targets[index].offsets) {
                const auto* material = std::get_if<types::MaterialMorphOffset>(&offset);
                if (material != nullptr) {
                    auto result = accumulate(*material, effective.unwrap()[index], multiply, add);
                    if (result.is_err()) {
                        return std::move(result).unwrap_err();
                    }
                }
            }
        }
        std::vector<types::UniformMaterial> result;
        for (std::size_t index = 0; index < materials.size(); ++index) {
            auto uniform = types::make_uniform_material(materials[index]);
            if (!apply_factors(uniform, multiply[index], add[index])) {
                return foundation::Error(
                    MorphError::InvalidWeight, "Non-finite material morph result");
            }
            result.push_back(uniform);
        }
        return result;
    }
} // namespace enishi::core
