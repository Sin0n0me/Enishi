#include "vrm_expression_uv.h"
#include "vrm_schema.h"
#include <cmath>
#include <set>

namespace enishi::assets_system {
    namespace {
        bool add_transform(VRMContext& context,
            types::MorphTarget& target,
            const VRMJson& bind,
            glm::vec2 scale,
            glm::vec2 offset) {
            if (!std::isfinite(scale.x) || !std::isfinite(scale.y) || !std::isfinite(offset.x) ||
                !std::isfinite(offset.y)) {
                return context.fail("Non-finite VRM texture transform expression");
            }
            const auto& indices = std::get<std::vector<std::uint32_t>>(context.model.indices);
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
                const auto& material = context.model.materials[index];
                const auto& texture = source.pbr_metallic_roughness.base_color_texture;
                const auto rotation = texture.has_transform != 0 ? texture.transform.rotation : 0;
                std::set<std::uint32_t> vertices;
                for (std::size_t element = material.first_offset;
                     element < static_cast<std::size_t>(material.first_offset) + material.count;
                     ++element) {
                    vertices.insert(indices[element]);
                }
                for (const auto vertex : vertices) {
                    const auto raw = context.raw_uvs[vertex] * scale;
                    const auto& base =
                        std::get<types::Vertex>(context.model.vertices[vertex].front());
                    // UV delta = target_offset + R * target_scale * original_uv - base_uv.
                    const auto transformed =
                        offset + glm::vec2(std::cos(rotation) * raw.x - std::sin(rotation) * raw.y,
                                     std::sin(rotation) * raw.x + std::cos(rotation) * raw.y);
                    target.offsets.emplace_back(
                        types::UVMorphOffset{vertex, 0, glm::vec4(transformed - base.uv, 0, 0)});
                }
            }
            return found || context.fail("VRM texture expression references a missing material");
        }
    } // namespace

    bool bind_vrm_texture_transforms(
        VRMContext& context, types::MorphTarget& target, const VRMJson& expression) {
        const auto& binds = expression[context.legacy ? vrm_schema::legacy_key::MATERIAL_VALUES
                                                      : vrm_schema::key::TEXTURE_TRANSFORM_BINDS];
        for (const auto& bind : binds.elements) {
            glm::vec2 scale(1);
            glm::vec2 offset(0);
            if (context.legacy) {
                if (bind[vrm_schema::legacy_key::PROPERTY_NAME].value !=
                    vrm_schema::legacy_material_property::MAIN_TEXTURE_TRANSFORM) {
                    continue;
                }
                const auto& value = bind[vrm_schema::key::TARGET_VALUE].elements;
                if (value.size() != 4) {
                    return context.fail("Invalid VRM 0 texture expression");
                }
                scale = {value[0].number(), value[1].number()};
                // Unity uses bottom-left UVs: glTF offset_y = 1 - scale_y - Unity offset_y.
                offset = {value[2].number(), 1 - scale.y - value[3].number()};
            } else {
                if (bind[vrm_schema::key::SCALE].valid) {
                    const auto& value = bind[vrm_schema::key::SCALE].elements;
                    if (value.size() != 2) {
                        return context.fail("Invalid VRM texture expression scale");
                    }
                    scale = {value[0].number(), value[1].number()};
                }
                if (bind[vrm_schema::key::OFFSET].valid) {
                    const auto& value = bind[vrm_schema::key::OFFSET].elements;
                    if (value.size() != 2) {
                        return context.fail("Invalid VRM texture expression offset");
                    }
                    offset = {value[0].number(), value[1].number()};
                }
            }
            if (!add_transform(context, target, bind, scale, offset)) {
                return false;
            }
        }
        return true;
    }
} // namespace enishi::assets_system
