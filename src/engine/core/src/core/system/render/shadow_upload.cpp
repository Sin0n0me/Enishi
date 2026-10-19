#include "shadow_upload.h"
#include <component/model_component.h>
#include <component/skinning_component.h>

namespace enishi::core {
    namespace {
        foundation::Result<void, platform::RenderError> write_shadow(
            platform::IResourceUpdater& updater, const types::UniformShadow& shadow) {
            auto& resource = updater.get_resource();
            const auto data = resource.get_render_data();
            if (data.stride != sizeof(types::UniformShadow) ||
                data.byte_width() != sizeof(types::UniformShadow)) {
                return foundation::Error(platform::RenderError::ResolveError,
                    "Shadow buffer capacity or stride mismatch");
            }
            resource.update(shadow, 0);
            updater.on_update();
            return {};
        }

        foundation::Result<void, platform::RenderError> upload_model_shadow(
            platform::IRenderer& renderer,
            const component::ModelComponent& model,
            const types::UniformShadow& shadow) {
            const auto mapped = renderer.get_handle_mapper()->get(model.render_handle);
            if (mapped.is_none()) {
                return foundation::Error(
                    platform::RenderError::ResolveError, "Model mesh handle is missing");
            }
            auto* resources = renderer.get_resource_accessor()->get_resource_accessor();
            const auto mesh =
                resources->get_mesh_accessor()->get_mesh_handle(mapped.unwrap().resource);
            if (mesh.is_none()) {
                return foundation::Error(
                    platform::RenderError::ResolveError, "Model mesh resource is missing");
            }
            const auto& uniforms = mesh.unwrap().uniform_buffers;
            const auto found = uniforms.find(types::UniformShadow::UNIFORM_NAME);
            if (found == uniforms.end() || found->second.empty()) {
                return foundation::Error(
                    platform::RenderError::ResolveError, "Model shadow uniform is missing");
            }
            for (const auto handle : found->second) {
                const auto buffer = resources->get_buffer_accessor()->get_buffer(handle);
                if (buffer.is_none() || buffer.unwrap() == nullptr) {
                    return foundation::Error(
                        platform::RenderError::ResolveError, "Shadow updater is missing");
                }
                auto result = write_shadow(*buffer.unwrap(), shadow);
                if (result.is_err()) {
                    return result;
                }
            }
            return {};
        }
    } // namespace

    foundation::Result<void, platform::RenderError> upload_shadow(platform::IRenderer& renderer,
        ecs::Registry& registry,
        const types::UniformCamera& camera) {
        types::ShadowBounds light_bounds;
        const auto light_world = types::shadow_light_view() * camera.world;
        for (auto [entity, model] : registry.view<component::ModelComponent>()) {
            if (!model.visible) {
                continue;
            }
            const auto skinning = registry.get<component::SkinningComponent>(entity);
            if (skinning.is_none() || model.shadow_bone_bounds.empty()) {
                light_bounds.include(model.shadow_model_bounds, light_world);
                continue;
            }
            const auto& matrices = skinning.unwrap().skinning_matrices;
            for (std::size_t bone = 0; bone < model.shadow_bone_bounds.size(); ++bone) {
                if (!(bone < matrices.size()) || !(bone < model.bind_bone.size())) {
                    continue;
                }
                // bone-local -> world = world * skinning * bind_global.
                light_bounds.include(model.shadow_bone_bounds[bone],
                    light_world * matrices[bone] * model.bind_bone[bone].global);
            }
        }
        const auto shadow = types::fit_shadow_projection(light_bounds);
        for (auto [entity, model] : registry.view<component::ModelComponent>()) {
            if (!model.visible) {
                continue;
            }
            auto result = upload_model_shadow(renderer, model, shadow);
            if (result.is_err()) {
                return result;
            }
        }
        return {};
    }
} // namespace enishi::core
