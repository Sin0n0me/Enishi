#include "shadow_receiver.h"
#include <glm/ext/matrix_transform.hpp>

namespace enishi::types {
    ShadowReceiverProjection fit_shadow_receiver(
        const ShadowBounds& world_bounds, ShadowBounds light_bounds, const UniformCamera& camera) {
        ShadowReceiverProjection result{camera, fit_shadow_projection(light_bounds)};
        // A zero-area receiver produces no fragments when no visible caster can project onto it.
        result.camera.world = glm::scale(glm::mat4(1), glm::vec3(0));
        result.camera.mvp = camera.projection * camera.view * result.camera.world;
        if (!world_bounds.valid) {
            return result;
        }
        ShadowBounds view_bounds;
        view_bounds.include(world_bounds, camera.view);
        const auto light_view = shadow_light_view();
        // The +Z basis of inverse(light_view) is the direction travelled by light.
        const auto ray = glm::mat3(camera.view) * glm::vec3(glm::inverse(light_view)[2]);
        constexpr float PARALLEL_EPSILON = 0.0001f;
        if (!(ray.z > PARALLEL_EPSILON)) {
            return result;
        }
        constexpr float WALL_GAP_RATIO = 0.05f;
        constexpr float MINIMUM_GAP = 0.1f;
        const auto gap =
            (glm::max)((view_bounds.maximum.y - view_bounds.minimum.y) * WALL_GAP_RATIO,
                MINIMUM_GAP);
        const auto wall_depth = view_bounds.maximum.z + gap;
        const auto inverse_view = glm::inverse(camera.view);
        ShadowBounds wall_bounds;
        constexpr unsigned CORNER_COUNT = 8;
        for (unsigned corner = 0; corner < CORNER_COUNT; ++corner) {
            const glm::vec3 point{
                (corner & 1u) != 0 ? view_bounds.maximum.x : view_bounds.minimum.x,
                (corner & 2u) != 0 ? view_bounds.maximum.y : view_bounds.minimum.y,
                (corner & 4u) != 0 ? view_bounds.maximum.z : view_bounds.minimum.z};
            // Ray/plane intersection: q = p + ray * (wall_z - p.z) / ray.z.
            const auto projected = point + ray * ((wall_depth - point.z) / ray.z);
            wall_bounds.include(projected);
            light_bounds.include(glm::vec3(light_view * inverse_view * glm::vec4(projected, 1)));
        }
        constexpr float HALF = 0.5f;
        const auto center = (wall_bounds.minimum + wall_bounds.maximum) * HALF;
        const auto size =
            glm::max((wall_bounds.maximum - wall_bounds.minimum) * HALF, glm::vec3(MINIMUM_GAP));
        // Unit square -> camera-facing wall -> world; clip = projection * view * wall_world * p.
        result.camera.world = inverse_view * glm::translate(glm::mat4(1), center) *
                              glm::scale(glm::mat4(1), glm::vec3(size.x, size.y, 1));
        result.camera.mvp = camera.projection * camera.view * result.camera.world;
        result.shadow = fit_shadow_projection(light_bounds);
        return result;
    }
} // namespace enishi::types
