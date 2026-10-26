#include "obb_wireframe.h"
#include <array>
#include <cmath>

namespace enishi::debug {
    namespace {
        constexpr std::size_t AXIS_COUNT = 3;
        constexpr std::size_t CORNER_COUNT = 8;
        constexpr float MINIMUM_CLIP_W = 1e-6f;
        constexpr float NDC_TO_VIEWPORT_SCALE = 0.5f;

        bool finite(const glm::vec4& value) {
            return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z) &&
                   std::isfinite(value.w);
        }

        bool clip_edge(glm::vec4& start, glm::vec4& end) {
            // HLSL clip volume: -w <= x,y <= w and 0 <= z <= w.
            const std::array<glm::vec4, 6> planes{glm::vec4(1, 0, 0, 1),
                glm::vec4(-1, 0, 0, 1),
                glm::vec4(0, 1, 0, 1),
                glm::vec4(0, -1, 0, 1),
                glm::vec4(0, 0, 1, 0),
                glm::vec4(0, 0, -1, 1)};
            for (const auto& plane : planes) {
                const float start_distance = glm::dot(plane, start);
                const float end_distance = glm::dot(plane, end);
                if (start_distance < 0.0f && end_distance < 0.0f) {
                    return false;
                }
                if ((start_distance < 0.0f) != (end_distance < 0.0f)) {
                    // Plane intersection: p(t) = a + t*(b-a), t = d(a)/(d(a)-d(b)).
                    const auto intersection =
                        start + (end - start) * (start_distance / (start_distance - end_distance));
                    if (start_distance < 0.0f) {
                        start = intersection;
                    } else {
                        end = intersection;
                    }
                }
            }
            return finite(start) && finite(end) && start.w > MINIMUM_CLIP_W &&
                   end.w > MINIMUM_CLIP_W;
        }

        glm::vec2 to_viewport(const glm::vec4& clip) {
            // u = (x/w + 1)/2, v = (1 - y/w)/2; UI applies viewport size and DPI.
            return {(clip.x / clip.w + 1.0f) * NDC_TO_VIEWPORT_SCALE,
                (1.0f - clip.y / clip.w) * NDC_TO_VIEWPORT_SCALE};
        }
    } // namespace

    void append_obb_wireframe(
        const types::OBB& obb, const glm::mat4& view_projection, std::vector<OverlayLine>& lines) {
        std::array<glm::vec4, CORNER_COUNT> corners{};
        for (std::size_t corner = 0; corner < CORNER_COUNT; ++corner) {
            auto position = obb.center;
            for (std::size_t axis = 0; axis < AXIS_COUNT; ++axis) {
                const float sign = (corner & (std::size_t{1} << axis)) == 0 ? -1.0f : 1.0f;
                // Corner = center + sum(sign[i] * half_extent[i] * axis[i]).
                position +=
                    sign * obb.half_extent[static_cast<glm::length_t>(axis)] * obb.axis[axis];
            }
            corners[corner] = view_projection * glm::vec4(position, 1.0f);
            if (!finite(corners[corner])) {
                return;
            }
        }
        for (std::size_t corner = 0; corner < CORNER_COUNT; ++corner) {
            for (std::size_t axis = 0; axis < AXIS_COUNT; ++axis) {
                const auto edge_bit = std::size_t{1} << axis;
                if ((corner & edge_bit) != 0) {
                    continue;
                }
                // Neighbors differ in one sign bit; visit each of the twelve edges once.
                auto start = corners[corner];
                auto end = corners[corner | edge_bit];
                if (clip_edge(start, end)) {
                    lines.push_back({to_viewport(start), to_viewport(end)});
                }
            }
        }
    }
} // namespace enishi::debug
