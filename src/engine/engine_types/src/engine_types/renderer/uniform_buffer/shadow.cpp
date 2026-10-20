#include "shadow.h"
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>

namespace enishi::types {
    void ShadowBounds::include(glm::vec3 point) {
        this->minimum = glm::min(this->minimum, point);
        this->maximum = glm::max(this->maximum, point);
        this->valid = true;
    }

    void ShadowBounds::include(const ShadowBounds& bounds, const glm::mat4& transform) {
        if (!bounds.valid) {
            return;
        }
        constexpr unsigned CORNER_COUNT = 8;
        for (unsigned corner = 0; corner < CORNER_COUNT; ++corner) {
            const glm::vec3 point{(corner & 1u) != 0 ? bounds.maximum.x : bounds.minimum.x,
                (corner & 2u) != 0 ? bounds.maximum.y : bounds.minimum.y,
                (corner & 4u) != 0 ? bounds.maximum.z : bounds.minimum.z};
            // An affine transform maps an AABB into the convex hull of its eight corners.
            this->include(glm::vec3(transform * glm::vec4(point, 1)));
        }
    }

    glm::mat4 shadow_light_view() {
        // Same surface-to-light direction as common/shadow_light.hlsl; independent of camera.
        const glm::vec3 direction = glm::normalize(glm::vec3(-0.35f, 0.8f, -1.0f));
        return glm::lookAtLH(glm::vec3(0), -direction, glm::vec3(0, 1, 0));
    }

    UniformShadow fit_shadow_projection(const ShadowBounds& light_bounds) {
        if (!light_bounds.valid) {
            return {};
        }
        constexpr float BOUNDS_MARGIN = 0.1f;
        constexpr float MINIMUM_MARGIN = 0.1f;
        const auto margin = glm::max((light_bounds.maximum - light_bounds.minimum) * BOUNDS_MARGIN,
            glm::vec3(MINIMUM_MARGIN));
        const auto minimum = light_bounds.minimum - margin;
        const auto maximum = light_bounds.maximum + margin;
        // light_clip = ortho_LH_[0,1](light-space bounds) * light_view * world_position.
        return {glm::orthoLH_ZO(minimum.x, maximum.x, minimum.y, maximum.y, minimum.z, maximum.z) *
                shadow_light_view()};
    }
} // namespace enishi::types
