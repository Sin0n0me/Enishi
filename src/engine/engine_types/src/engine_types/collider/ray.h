#pragma once
#include "../input/input_types.h"
#include "../renderer/uniform_buffer/camera.h"
#include "../window/window_types.h"
#include <glm/glm.hpp>

namespace enishi::types {
    struct Ray {
        glm::vec3 origin;
        glm::vec3 direction;

        [[nodiscard]] static Ray make_ray_from_screen(const WindowSize& window_size,
            const ClientMousePosition& mouse_position,
            const UniformCamera& camera);
    };
} // namespace enishi::types