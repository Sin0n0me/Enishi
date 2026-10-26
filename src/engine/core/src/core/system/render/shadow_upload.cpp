#include "shadow_upload.h"
#include <component/model_component.h>
#include <component/skinning_component.h>
#include <engine_types/renderer/uniform_buffer/shadow_receiver.h>
#include <renderer/common/upload/camera_upload.h>
#include <renderer/common/upload/shadow_uniform_upload.h>

namespace enishi::core {
    foundation::Result<void, platform::RenderError> upload_shadow(platform::IRenderer& renderer,
        ecs::Registry& registry,
        const types::UniformCamera& camera,
        std::span<const types::RenderHandle> receivers,
        bool self_shadow_enabled) {
        types::ShadowBounds light_bounds;
        types::ShadowBounds world_bounds;
        const auto light_world = types::shadow_light_view() * camera.world;
        for (auto [entity, model] : registry.view<component::ModelComponent>()) {
            if (!model.visible) {
                continue;
            }
            const auto skinning = registry.get<component::SkinningComponent>(entity);
            if (skinning.is_none() || model.shadow_bone_bounds.empty()) {
                light_bounds.include(model.shadow_model_bounds, light_world);
                world_bounds.include(model.shadow_model_bounds, camera.world);
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
                world_bounds.include(model.shadow_bone_bounds[bone],
                    camera.world * matrices[bone] * model.bind_bone[bone].global);
            }
        }
        const auto receiver = types::fit_shadow_receiver(world_bounds, light_bounds, camera);
        const auto shadow =
            receivers.empty() ? types::fit_shadow_projection(light_bounds) : receiver.shadow;
        for (const auto mesh : receivers) {
            auto result = renderer::upload_camera(renderer, mesh, receiver.camera);
            if (result.is_err()) {
                return result;
            }
            result = renderer::upload_model_shadow(renderer, mesh, shadow);
            if (result.is_err()) {
                return result;
            }
        }
        for (auto [entity, model] : registry.view<component::ModelComponent>()) {
            if (!model.visible) {
                continue;
            }
            auto model_shadow = shadow;
            model_shadow.options.x = self_shadow_enabled ? 0.0f : 1.0f;
            auto result =
                renderer::upload_model_shadow(renderer, model.render_handle, model_shadow);
            if (result.is_err()) {
                return result;
            }
        }
        return {};
    }
} // namespace enishi::core
