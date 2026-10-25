#include "vrm_material.h"
#include "../../../texture/texture_loader.h"
#include <engine_types/renderer/texture/model_texture.h>
#include <engine_types/renderer/uniform_buffer/material.h>
#include <glm/gtc/type_ptr.hpp>

namespace enishi::assets_system {
    namespace {
        bool attach_texture(
            VRMContext& context, types::Material& material, const cgltf_texture_view& texture) {
            if (texture.texture == nullptr) {
                return true;
            }
            const auto* image = texture.texture->image;
            if (image == nullptr || image->buffer_view == nullptr || image->uri != nullptr) {
                return context.fail("VRM requires embedded PNG/JPEG textures");
            }
            if (texture.texcoord != 0 ||
                (texture.has_transform != 0 && texture.transform.has_texcoord != 0 &&
                    texture.transform.texcoord != 0)) {
                return context.fail("VRM base color texture requires TEXCOORD_0");
            }
            const auto key =
                context.model.path / ("image_" + std::to_string(image - context.source.images));
            if (!context.model.textures.contains(key)) {
                const auto* bytes = cgltf_buffer_view_data(image->buffer_view);
                if (bytes == nullptr) {
                    return context.fail("Missing VRM image buffer");
                }
                auto decoded = TextureLoader::load_memory({bytes, image->buffer_view->size});
                if (decoded.is_err()) {
                    return context.fail(decoded.unwrap_err().get_message());
                }
                context.model.textures.emplace(key, std::move(decoded).unwrap_mut());
            }
            material.textures.push_back({key,
                types::ModelTexture::MODEL_TEXTURE_NAME,
                types::ModelTexture::MODEL_SAMPLER_NAME});
            return true;
        }
    } // namespace

    bool make_vrm_material(VRMContext& context,
        const cgltf_material* source,
        std::uint32_t first,
        std::uint32_t count) {
        types::Material material{};
        material.name = types::UniformMaterial::UNIFORM_NAME;
        material.first_offset = first;
        material.count = count;
        material.instance_count = 1;
        material.cast_shadow = true;
        material.receive_shadow = true;
        // Basic VRM rendering uses base color; MToon lighting is not approximated as MMD lighting.
        material.unlit = true;
        material.alpha_mode = types::MaterialAlphaMode::Opaque;
        glm::vec4 color(1);
        if (source != nullptr) {
            material.display_name = source->name != nullptr ? source->name : "VRM material";
            material.double_sided = source->double_sided != 0;
            material.alpha_cutoff = source->alpha_cutoff;
            if (source->alpha_mode == cgltf_alpha_mode_mask) {
                material.alpha_mode = types::MaterialAlphaMode::Mask;
            } else if (source->alpha_mode == cgltf_alpha_mode_blend) {
                material.alpha_mode = types::MaterialAlphaMode::Blend;
            }
            if (source->has_pbr_metallic_roughness != 0) {
                color = glm::make_vec4(source->pbr_metallic_roughness.base_color_factor);
                if (!attach_texture(
                        context, material, source->pbr_metallic_roughness.base_color_texture)) {
                    return false;
                }
            }
        }
        material.variants = {types::Diffuse{color},
            types::Specular{glm::vec3(0), 1},
            types::Ambient{glm::vec3(1)},
            glm::vec1(0),
            glm::vec1(0),
            glm::vec1(0)};
        context.model.materials.push_back(std::move(material));
        context.material_sources.push_back(
            source != nullptr ? static_cast<std::size_t>(source - context.source.materials)
                              : context.source.materials_count);
        return true;
    }
} // namespace enishi::assets_system
