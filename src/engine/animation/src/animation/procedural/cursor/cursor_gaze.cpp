#include "cursor_gaze.h"
#include <algorithm>
#include <cmath>
#include <glm/gtc/matrix_transform.hpp>
#include <utility>

namespace enishi::animation {
    using BodyBone = CursorPoseState::BodyBone;
    namespace {
        constexpr float EPSILON = 0.00001f;
        constexpr float PROXIMITY_RATIO = 0.18f;
        constexpr float TARGET_DEPTH_RATIO = 0.12f;
        constexpr float GAZE_DEPTH_RATIO = 0.25f;
        constexpr float FACE_HEIGHT_RATIO = 0.5f;
    } // namespace
    std::optional<glm::vec3> CursorGaze::target(
        const CursorPoseState& state, const CursorRay& cursor) {
        if (state.transforms.globals.empty()) {
            return std::nullopt;
        }
        auto low = glm::vec3(state.transforms.globals.front()[3]);
        auto high = low;
        for (const auto& global : state.transforms.globals) {
            low = (glm::min)(low, glm::vec3(global[3]));
            high = (glm::max)(high, glm::vec3(global[3]));
        }
        const float height = high.y - low.y;
        const float denominator = glm::dot(cursor.ray.direction, cursor.plane_normal);
        if (!(height > EPSILON) || !(std::abs(denominator) > EPSILON)) {
            return std::nullopt;
        }
        // Intersect the cursor ray with a camera-facing plane just in front of the body:
        // t = dot(center - origin, normal) / dot(direction, normal), target = origin + t*direction.
        const auto center = (low + high) * 0.5f - cursor.plane_normal * height * TARGET_DEPTH_RATIO;
        const float distance =
            glm::dot(center - cursor.ray.origin, cursor.plane_normal) / denominator;
        const auto point = cursor.ray.origin + cursor.ray.direction * distance;
        if (!(distance > 0) || !std::isfinite(distance)) {
            return std::nullopt;
        }
        // Ignore depth in the proximity test: the target plane is deliberately in front.
        const auto planar = glm::vec2(point);
        auto proximity_high = high;
        if (state.config.jump_enabled) {
            proximity_high.y += height * state.config.jump_height_ratio;
        }
        const auto closest = glm::clamp(planar, glm::vec2(low), glm::vec2(proximity_high));
        if (glm::length(planar - closest) > height * PROXIMITY_RATIO) {
            return std::nullopt;
        }
        return point;
    }

    std::optional<glm::vec3> CursorGaze::gaze_offset(
        const CursorPoseState& state, const CursorRay& cursor) {
        const auto head = state.body_bones[std::to_underlying(BodyBone::BodyHead)];
        const auto neck = state.body_bones[std::to_underlying(BodyBone::BodyNeck)];
        const auto anchor = head < state.transforms.globals.size() ? head : neck;
        if (!(anchor < state.transforms.globals.size())) {
            return std::nullopt;
        }
        auto center = glm::vec3(state.transforms.globals[anchor][3]);
        if (state.eyes[0] < state.transforms.globals.size() &&
            state.eyes[1] < state.transforms.globals.size()) {
            center = (glm::vec3(state.transforms.globals[state.eyes[0]][3]) +
                         glm::vec3(state.transforms.globals[state.eyes[1]][3])) *
                     0.5f;
        } else if (head < state.transforms.globals.size() &&
                   neck < state.transforms.globals.size()) {
            // Without eye bones, estimate the face center half a neck-to-head length
            // above the head pivot: c = head + up * |head-neck|/2.
            const auto up = glm::mat3(state.transforms.globals[head]) * state.head_local_up;
            if (glm::length(up) > EPSILON) {
                center += glm::normalize(up) * FACE_HEIGHT_RATIO *
                          glm::length(center - glm::vec3(state.transforms.globals[neck][3]));
            }
        }
        auto low = glm::vec3(state.transforms.globals.front()[3]);
        auto high = low;
        for (const auto& global : state.transforms.globals) {
            low = (glm::min)(low, glm::vec3(global[3]));
            high = (glm::max)(high, glm::vec3(global[3]));
        }
        const float depth = (high.y - low.y) * GAZE_DEPTH_RATIO;
        const float denominator = glm::dot(cursor.ray.direction, cursor.plane_normal);
        if (!(depth > EPSILON) || !(std::abs(denominator) > EPSILON)) {
            return std::nullopt;
        }
        // Intersect a camera-facing plane THROUGH the face, not the torso:
        // t=dot(c-origin,n)/dot(ray,n), offset=(origin+t*ray-c)/depth.
        // The projected face center therefore has zero offset at every camera angle.
        const float distance =
            glm::dot(center - cursor.ray.origin, cursor.plane_normal) / denominator;
        if (!(distance > 0) || !std::isfinite(distance)) {
            return std::nullopt;
        }
        return (cursor.ray.origin + distance * cursor.ray.direction - center) / depth;
    }

    void CursorGaze::look(const CursorPoseState& state,
        component::AnimationComponent& animation,
        CursorPoseState::Joint joint,
        const glm::vec3& offset,
        float yaw_limit,
        float pitch_limit) {
        const auto index = state.joints[std::to_underlying(joint)];
        if (!(index < state.transforms.globals.size())) {
            return;
        }
        const auto& global = state.transforms.globals[index];
        if (!(std::abs(glm::determinant(glm::mat3(global))) > EPSILON)) {
            return;
        }
        auto axes = glm::mat3(global);
        for (glm::length_t column = 0; column < axes.length(); ++column) {
            axes[column] = glm::normalize(axes[column]);
        }
        const auto local = glm::inverse(axes) * offset;
        // Keep the virtual goal strictly ahead: d=(offset.x,offset.y,-1).
        // Unlike a nearby world-space point, this cannot cross atan2's rear-facing branch cut.
        const glm::vec3 direction(local.x, local.y, -1.0f);
        // MMD faces local -Z. yaw=atan2(-x,-z), pitch=atan2(y,sqrt(x*x+z*z)).
        const float yaw = std::clamp(std::atan2(-direction.x, -direction.z),
            -glm::radians(yaw_limit),
            glm::radians(yaw_limit));
        const float pitch =
            std::clamp(std::atan2(direction.y, glm::length(glm::vec2(direction.x, direction.z))),
                -glm::radians(pitch_limit),
                glm::radians(pitch_limit));
        animation.animation[index].rotation *=
            glm::angleAxis(yaw, glm::vec3(0, 1, 0)) * glm::angleAxis(pitch, glm::vec3(1, 0, 0));
    }
} // namespace enishi::animation
