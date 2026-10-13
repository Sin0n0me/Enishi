#include "cursor_pose.h"
#include <algorithm>
#include <cmath>
#include <glm/gtc/matrix_transform.hpp>
#include <initializer_list>
#include <limits>
#include <string_view>

namespace enishi::core {
    namespace {
        constexpr glm::quat IDENTITY(1, 0, 0, 0);
        constexpr float EPSILON = 0.00001f;
        constexpr float RESPONSE_RATE = 8.0f;
        constexpr float PROXIMITY_RATIO = 0.18f;
        constexpr float TARGET_DEPTH_RATIO = 0.12f;
        constexpr float REACH_RATIO = 0.98f;
        constexpr float ARM_SWITCH_RATIO = 0.8f;
        constexpr float NECK_YAW = 20.0f;
        constexpr float NECK_PITCH = 12.0f;
        constexpr float HEAD_YAW = 40.0f;
        constexpr float HEAD_PITCH = 25.0f;
        constexpr float ARM_ANGLE = 120.0f;
        constexpr float ELBOW_ANGLE = 150.0f;

        types::BoneIndex find_bone(
            const component::ModelComponent& model, std::initializer_list<std::string_view> names) {
            for (const auto name : names) {
                const auto found = std::ranges::find(model.bone_names, name);
                if (found != model.bone_names.end()) {
                    return static_cast<types::BoneIndex>(found - model.bone_names.begin());
                }
            }
            return types::INVALID_BONE_INDEX;
        }

        bool descendant(const component::ModelComponent& model,
            types::BoneIndex child,
            types::BoneIndex parent) {
            for (std::size_t step = 0; step < model.bone_node.size(); ++step) {
                if (!(child < model.bone_node.size())) {
                    return false;
                }
                child = model.bone_node[child].parent;
                if (child == parent && parent < model.bone_node.size()) {
                    return true;
                }
            }
            return false;
        }

        glm::quat limited(glm::quat rotation, float degrees) {
            if (rotation.w < 0) {
                rotation = -rotation;
            }
            // Unit quaternion angle = 2*acos(w); interpolate to cap the additive rotation.
            const float angle = 2.0f * std::acos(std::clamp(rotation.w, 0.0f, 1.0f));
            const float limit = glm::radians(degrees);
            return angle > limit ? glm::slerp(IDENTITY, rotation, limit / angle) : rotation;
        }

        glm::quat between(glm::vec3 from, glm::vec3 to) {
            if (!(glm::length(from) > EPSILON) || !(glm::length(to) > EPSILON)) {
                return IDENTITY;
            }
            from = glm::normalize(from);
            to = glm::normalize(to);
            const float cosine = std::clamp(glm::dot(from, to), -1.0f, 1.0f);
            if (cosine < -1.0f + EPSILON) {
                auto axis = glm::cross(from, glm::vec3(0, 1, 0));
                if (!(glm::length(axis) > EPSILON)) {
                    axis = glm::cross(from, glm::vec3(1, 0, 0));
                }
                return glm::angleAxis(glm::pi<float>(), glm::normalize(axis));
            }
            // Shortest-arc quaternion q = normalize((1 + dot(a,b), cross(a,b))).
            return glm::normalize(glm::quat(1.0f + cosine, glm::cross(from, to)));
        }
    } // namespace

    CursorPose::CursorPose(const component::ModelComponent& model)
        : joints{find_bone(model, {"首", "neck", "Neck"}),
              find_bone(model, {"頭", "head", "Head"}),
              find_bone(model, {"左腕", "leftUpperArm", "LeftArm"}),
              find_bone(model, {"左ひじ", "左肘", "leftLowerArm", "LeftForeArm"}),
              find_bone(model, {"右腕", "rightUpperArm", "RightArm"}),
              find_bone(model, {"右ひじ", "右肘", "rightLowerArm", "RightForeArm"})}
        , wrists{find_bone(model, {"左手首", "leftHand", "LeftHand"}),
              find_bone(model, {"右手首", "rightHand", "RightHand"})} {
        this->offsets.fill(IDENTITY);
        this->originals.fill(IDENTITY);
        for (auto& index : this->joints) {
            for (const auto& body : model.rigid_bodies) {
                if (body.relate_bone_index == index &&
                    body.kind != types::RigidBodyKind::Kinematic) {
                    index = types::INVALID_BONE_INDEX;
                }
            }
            for (const auto& constraint : model.bone_constraints.constraints) {
                if (constraint.bone == index && constraint.rotation_axis.has_value()) {
                    index = types::INVALID_BONE_INDEX;
                }
            }
        }
        for (std::size_t side = 0; side < this->wrists.size(); ++side) {
            const auto arm = side == 0 ? LeftArm : RightArm;
            const auto elbow = side == 0 ? LeftElbow : RightElbow;
            if (!descendant(model, this->joints[elbow], this->joints[arm]) ||
                !descendant(model, this->wrists[side], this->joints[elbow])) {
                this->joints[arm] = types::INVALID_BONE_INDEX;
                this->joints[elbow] = types::INVALID_BONE_INDEX;
                this->wrists[side] = types::INVALID_BONE_INDEX;
            }
        }
        // Build a parent-first traversal without assuming source bone indices are sorted.
        std::vector<std::vector<types::BoneIndex>> children(model.bone_node.size());
        for (types::BoneIndex index = 0; index < model.bone_node.size(); ++index) {
            const auto parent = model.bone_node[index].parent;
            if (parent < children.size()) {
                children[parent].push_back(index);
            } else {
                this->traversal.push_back(index);
            }
        }
        for (std::size_t cursor = 0; cursor < this->traversal.size(); ++cursor) {
            const auto& next = children[this->traversal[cursor]];
            this->traversal.insert(this->traversal.end(), next.begin(), next.end());
        }
        this->globals.resize(model.bone_node.size(), glm::mat4(1));
        this->external.resize(model.bone_node.size(), glm::mat4(1));
    }

    void CursorPose::update_globals(
        const component::ModelComponent& model, const component::AnimationComponent& animation) {
        for (const auto index : this->traversal) {
            const auto& local = animation.animation[index];
            const auto parent = model.bone_node[index].parent;
            const auto parent_global =
                parent < this->globals.size() ? this->globals[parent] : glm::mat4(1);
            // G = external * G_parent * T * R * S, matching the skinning hierarchy.
            this->globals[index] = this->external[index] * parent_global *
                                   glm::translate(glm::mat4(1), local.position) *
                                   glm::mat4_cast(local.rotation) *
                                   glm::scale(glm::mat4(1), local.scale);
        }
    }

    std::optional<glm::vec3> CursorPose::target(const CursorRay& cursor) const {
        if (this->globals.empty()) {
            return std::nullopt;
        }
        auto low = glm::vec3(this->globals.front()[3]);
        auto high = low;
        for (const auto& global : this->globals) {
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
        const auto closest = glm::clamp(planar, glm::vec2(low), glm::vec2(high));
        if (glm::length(planar - closest) > height * PROXIMITY_RATIO) {
            return std::nullopt;
        }
        return point;
    }

    void CursorPose::look(component::AnimationComponent& animation,
        Joint joint,
        const glm::vec3& target,
        float yaw_limit,
        float pitch_limit) {
        const auto index = this->joints[joint];
        if (!(index < this->globals.size())) {
            return;
        }
        const auto& global = this->globals[index];
        if (!(std::abs(glm::determinant(glm::mat3(global))) > EPSILON)) {
            return;
        }
        const auto direction = glm::inverse(glm::mat3(global)) * (target - glm::vec3(global[3]));
        if (!(glm::length(direction) > EPSILON)) {
            return;
        }
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

    void CursorPose::aim(component::AnimationComponent& animation,
        Joint joint,
        const glm::vec3& from,
        const glm::vec3& to,
        float limit) {
        const auto index = this->joints[joint];
        const auto axes = glm::mat3(this->globals[index]);
        if (!(std::abs(glm::determinant(axes)) > EPSILON)) {
            return;
        }
        // Express the world-space direction correction in the joint's current local frame.
        const auto inverse = glm::inverse(axes);
        animation.animation[index].rotation *=
            limited(between(inverse * from, inverse * to), limit);
    }

    void CursorPose::reach(const component::ModelComponent& model,
        component::AnimationComponent& animation,
        const glm::vec3& target) {
        std::array<float, 2> distances{};
        for (std::size_t side = 0; side < this->wrists.size(); ++side) {
            const auto arm = this->joints[side == 0 ? LeftArm : RightArm];
            distances[side] = arm < this->globals.size()
                                  ? glm::length(target - glm::vec3(this->globals[arm][3]))
                                  : std::numeric_limits<float>::infinity();
        }
        const std::size_t nearest = distances[0] < distances[1] ? 0 : 1;
        if (!std::isfinite(distances[nearest])) {
            return;
        }
        if (!this->active_arm.has_value() ||
            distances[nearest] < distances[*this->active_arm] * ARM_SWITCH_RATIO) {
            this->active_arm = nearest;
        }
        const auto side = *this->active_arm;
        const auto arm = side == 0 ? LeftArm : RightArm;
        const auto elbow = side == 0 ? LeftElbow : RightElbow;
        const auto shoulder_position = glm::vec3(this->globals[this->joints[arm]][3]);
        const auto elbow_position = glm::vec3(this->globals[this->joints[elbow]][3]);
        const auto wrist_position = glm::vec3(this->globals[this->wrists[side]][3]);
        const float upper = glm::length(elbow_position - shoulder_position);
        const float lower = glm::length(wrist_position - elbow_position);
        const auto toward = target - shoulder_position;
        if (!(upper > EPSILON) || !(lower > EPSILON) || !(glm::length(toward) > EPSILON)) {
            return;
        }
        const float minimum = std::abs(upper - lower) + EPSILON;
        const float maximum = (upper + lower) * REACH_RATIO;
        if (!(minimum < maximum)) {
            return;
        }
        const float distance = std::clamp(glm::length(toward), minimum, maximum);
        const auto direction = glm::normalize(toward);
        // Two-bone IK: x=(a²-b²+d²)/(2d), h=sqrt(a²-x²), elbow=S+x*u+h*pole.
        const float along =
            (upper * upper - lower * lower + distance * distance) / (2.0f * distance);
        const float perpendicular = std::sqrt((std::max)(0.0f, upper * upper - along * along));
        auto pole = glm::vec3(0, -1, 0) - direction * glm::dot(glm::vec3(0, -1, 0), direction);
        if (!(glm::length(pole) > EPSILON)) {
            pole = glm::vec3(0, 0, -1) - direction * glm::dot(glm::vec3(0, 0, -1), direction);
        }
        const auto desired_elbow =
            shoulder_position + direction * along + glm::normalize(pole) * perpendicular;
        this->aim(animation,
            arm,
            elbow_position - shoulder_position,
            desired_elbow - shoulder_position,
            ARM_ANGLE);
        this->update_globals(model, animation);
        const auto moved_elbow = glm::vec3(this->globals[this->joints[elbow]][3]);
        const auto moved_wrist = glm::vec3(this->globals[this->wrists[side]][3]);
        const auto reachable = shoulder_position + direction * distance;
        this->aim(
            animation, elbow, moved_wrist - moved_elbow, reachable - moved_elbow, ELBOW_ANGLE);
    }

    void CursorPose::apply(const component::ModelComponent& model,
        component::AnimationComponent& animation,
        const std::optional<CursorRay>& cursor,
        float seconds) {
        if (this->applied || animation.animation.size() != this->globals.size() ||
            this->traversal.size() != this->globals.size()) {
            return;
        }
        for (std::size_t joint = 0; joint < Count; ++joint) {
            if (this->joints[joint] < animation.animation.size()) {
                this->originals[joint] = animation.animation[this->joints[joint]].rotation;
            }
        }
        std::fill(this->external.begin(), this->external.end(), glm::mat4(1));
        for (const auto& constraint : model.bone_constraints.constraints) {
            if (constraint.bone < this->external.size() &&
                constraint.external_transform_slot.has_value()) {
                const auto found =
                    model.external_transforms.find(*constraint.external_transform_slot);
                if (found != model.external_transforms.end()) {
                    this->external[constraint.bone] = found->second;
                }
            }
        }
        this->update_globals(model, animation);
        const auto point = cursor.has_value() ? this->target(*cursor) : std::nullopt;
        if (point.has_value()) {
            this->look(animation, Neck, *point, NECK_YAW, NECK_PITCH);
            this->update_globals(model, animation);
            this->look(animation, Head, *point, HEAD_YAW, HEAD_PITCH);
            this->update_globals(model, animation);
            this->reach(model, animation, *point);
        } else {
            this->active_arm.reset();
        }
        // Exponential smoothing alpha=1-exp(-rate*dt) is independent of frame rate.
        const float alpha = std::isfinite(seconds)
                                ? 1.0f - std::exp(-RESPONSE_RATE * (std::max)(0.0f, seconds))
                                : 0.0f;
        for (std::size_t joint = 0; joint < Count; ++joint) {
            const auto index = this->joints[joint];
            if (!(index < animation.animation.size())) {
                continue;
            }
            const auto desired = glm::normalize(
                glm::inverse(this->originals[joint]) * animation.animation[index].rotation);
            this->offsets[joint] = glm::normalize(glm::slerp(this->offsets[joint], desired, alpha));
            animation.animation[index].rotation =
                glm::normalize(this->originals[joint] * this->offsets[joint]);
        }
        this->applied = true;
    }

    void CursorPose::restore(component::AnimationComponent& animation) {
        if (!this->applied) {
            return;
        }
        for (std::size_t joint = 0; joint < Count; ++joint) {
            if (this->joints[joint] < animation.animation.size()) {
                animation.animation[this->joints[joint]].rotation = this->originals[joint];
            }
        }
        this->applied = false;
    }
} // namespace enishi::core
