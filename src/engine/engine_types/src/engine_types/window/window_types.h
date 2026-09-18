#pragma once
#include <cstdint>
#include <glm/glm.hpp>

namespace enishi::types {
    struct WindowSize {
        std::int32_t width;
        std::int32_t height;

        constexpr float aspect_ratio(void) const noexcept {
            return static_cast<float>(this->width) / static_cast<float>(this->height);
        }

        glm::vec2 to_glm_vec2(void) const noexcept {
            return glm::vec2{
                static_cast<float>(this->width),
                static_cast<float>(this->height),
            };
        }

        glm::ivec2 to_glm_ivec2(void) const noexcept {
            return glm::ivec2{this->width, this->height};
        }

        glm::uvec2 to_glm_uvec2(void) const noexcept {
            return glm::uvec2{this->width, this->height};
        }
    };

    struct WindowPosition {
        std::int32_t x;
        std::int32_t y;
    };
} // namespace enishi::types