#pragma once
#include <glm/glm.hpp>
#include <limits>

namespace enishi::types {
    struct ShadowBounds {
        glm::vec3 minimum{(std::numeric_limits<float>::max)()};
        glm::vec3 maximum{-(std::numeric_limits<float>::max)()};
        bool valid = false;
        void include(glm::vec3 point);
        void include(const ShadowBounds& bounds, const glm::mat4& transform);
    };

    struct UniformShadow {
        static constexpr const char* UNIFORM_NAME = "ShadowMap";
        glm::mat4 light_view_proj{1};
        // x disables self-shadow reception only; the caster and wall keep using the light matrix.
        glm::vec4 options{0};
    };

    glm::mat4 shadow_light_view();
    UniformShadow fit_shadow_projection(const ShadowBounds& light_bounds);
} // namespace enishi::types
