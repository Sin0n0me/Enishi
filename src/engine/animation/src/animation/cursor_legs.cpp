#include "cursor_pose.h"
#include <algorithm>
#include <cmath>
#include <initializer_list>
#include <string_view>

namespace enishi::animation {
    namespace {
        constexpr float EPSILON = 0.00001f;
        constexpr float KNEE_BEND_DEGREES = 25.0f;

        types::BoneIndex leg_bone(
            const component::ModelComponent& model, std::initializer_list<std::string_view> names) {
            for (const auto name : names) {
                const auto found = std::ranges::find(model.bone_names, name);
                if (found != model.bone_names.end()) {
                    return static_cast<types::BoneIndex>(found - model.bone_names.begin());
                }
            }
            return types::INVALID_BONE_INDEX;
        }

        bool under(const component::ModelComponent& model,
            types::BoneIndex bone,
            types::BoneIndex ancestor) {
            for (std::size_t step = 0;
                 step < model.bone_node.size() && bone < model.bone_node.size();
                 ++step) {
                if (bone == ancestor) {
                    return true;
                }
                bone = model.bone_node[bone].parent;
            }
            return false;
        }

        glm::quat orientation(const glm::mat4& matrix) {
            glm::mat3 axes(matrix);
            for (glm::length_t column = 0; column < axes.length(); ++column) {
                if (!(glm::length(axes[column]) > EPSILON)) {
                    return glm::quat(1, 0, 0, 0);
                }
                axes[column] = glm::normalize(axes[column]);
            }
            return glm::normalize(glm::quat_cast(axes));
        }

        glm::quat align(const glm::vec3& from, const glm::vec3& to) {
            if (!(glm::length(from) > EPSILON) || !(glm::length(to) > EPSILON)) {
                return glm::quat(1, 0, 0, 0);
            }
            const auto a = glm::normalize(from);
            const auto b = glm::normalize(to);
            if (glm::dot(a, b) < -1.0f + EPSILON) {
                auto axis = glm::cross(a, glm::vec3(1, 0, 0));
                if (!(glm::length(axis) > EPSILON)) {
                    axis = glm::cross(a, glm::vec3(0, 1, 0));
                }
                return glm::angleAxis(glm::pi<float>(), glm::normalize(axis));
            }
            // Shortest arc q = normalize((1 + dot(a,b), cross(a,b))).
            return glm::normalize(glm::quat(1.0f + glm::dot(a, b), glm::cross(a, b)));
        }
    } // namespace

    void CursorPose::initialize_legs(const component::ModelComponent& model) {
        this->legs = {{{leg_bone(model, {"左足", "leftUpperLeg", "LeftUpLeg"}),
                           leg_bone(model, {"左ひざ", "左膝", "leftLowerLeg", "LeftLeg"}),
                           leg_bone(model, {"左足首", "leftFoot", "LeftFoot"})},
            {leg_bone(model, {"右足", "rightUpperLeg", "RightUpLeg"}),
                leg_bone(model, {"右ひざ", "右膝", "rightLowerLeg", "RightLeg"}),
                leg_bone(model, {"右足首", "rightFoot", "RightFoot"})}}};
        bool valid = true;
        for (const auto& leg : this->legs) {
            valid = valid && leg[0] != leg[1] && leg[1] != leg[2] && under(model, leg[1], leg[0]) &&
                    under(model, leg[2], leg[1]);
            for (const auto bone : leg) {
                for (const auto& body : model.rigid_bodies) {
                    if (body.relate_bone_index == bone &&
                        body.kind != types::RigidBodyKind::Kinematic) {
                        valid = false;
                    }
                }
                for (const auto& constraint : model.bone_constraints.constraints) {
                    if (constraint.bone == bone && constraint.rotation_axis.has_value()) {
                        valid = false;
                    }
                }
            }
        }
        if (!valid) {
            for (auto& leg : this->legs) {
                leg.fill(types::INVALID_BONE_INDEX);
            }
            return;
        }
        for (const auto& leg : this->legs) {
            for (const auto bone : leg) {
                this->leg_originals.emplace_back(bone, component::AnimationBuffer{});
            }
        }
        for (const auto& ik : model.iks) {
            const auto& ccd = std::get<types::CCDIK>(ik.method);
            if (ccd.ik_bone < model.bone_node.size() &&
                (under(model, ccd.target, this->legs[0][0]) ||
                    under(model, ccd.target, this->legs[1][0])) &&
                std::ranges::find(this->leg_ik_goals, ccd.ik_bone) == this->leg_ik_goals.end()) {
                this->leg_ik_goals.push_back(ccd.ik_bone);
                this->leg_originals.emplace_back(ccd.ik_bone, component::AnimationBuffer{});
            }
        }
    }

    void CursorPose::bend_leg(const component::ModelComponent& model,
        component::AnimationComponent& animation,
        std::size_t side,
        const glm::mat4& ankle) {
        const auto& leg = this->legs[side];
        const auto hip = glm::vec3(this->globals[leg[0]][3]);
        const auto knee = glm::vec3(this->globals[leg[1]][3]);
        const auto foot = glm::vec3(this->globals[leg[2]][3]);
        const float upper = glm::length(knee - hip);
        const float lower = glm::length(foot - knee);
        const auto delta = glm::vec3(ankle[3]) - hip;
        const float distance = glm::length(delta);
        if (!(upper > EPSILON) || !(lower > EPSILON) || !(distance > EPSILON)) {
            return;
        }
        const auto bind = orientation(model.bind_bone[leg[1]].local);
        const auto hip_rotation = orientation(this->globals[leg[0]]);
        const auto knee_rotation = orientation(this->globals[leg[1]]);
        // Q_knee = Q_parent * Q_local. Remove the current local rotation before applying bind;
        // Q_parent may include helper bones, so it is not necessarily Q_hip.
        const auto knee_base =
            knee_rotation * glm::inverse(animation.animation[leg[1]].rotation) * bind;
        const auto upper_local = glm::inverse(knee_base) * (knee - hip);
        const auto lower_local = glm::inverse(knee_rotation) * (foot - knee);
        // For an X hinge, dot(u,Rx(theta)*v)=A*cos(theta)+B*sin(theta)+C.
        // The law of cosines gives that dot product as (distance^2-upper^2-lower^2)/2.
        const float a = upper_local.y * lower_local.y + upper_local.z * lower_local.z;
        const float b = upper_local.z * lower_local.y - upper_local.y * lower_local.z;
        const float c = upper_local.x * lower_local.x;
        const float radius = std::sqrt(a * a + b * b);
        if (!(radius > EPSILON)) {
            return;
        }
        const float cosine =
            std::clamp(((distance * distance - upper * upper - lower * lower) * 0.5f - c) / radius,
                -1.0f,
                1.0f);
        const float angle =
            std::clamp(std::atan2(b, a) - std::acos(cosine), -glm::pi<float>(), 0.0f);
        animation.animation[leg[1]].rotation = bind * glm::angleAxis(angle, glm::vec3(1, 0, 0));
        this->update_globals(model, animation);
        // Aim the whole bent leg at the ankle; all sideways adjustment belongs to the hip.
        const auto bent_leg = glm::vec3(this->globals[leg[2]][3]) - hip;
        animation.animation[leg[0]].rotation *=
            glm::inverse(hip_rotation) * align(bent_leg, delta) * hip_rotation;
        this->update_globals(model, animation);
        // Do not align the knee's positional projection to an extra pole. A bind leg can have
        // lateral offsets: its projection is then nonzero even at zero flex. Forcing that
        // projection into a chosen plane adds an arbitrary (up to 180-degree) hip twist.
        // The hinge angle fixes length and the shortest hip swing already reaches the ankle.
        // Preserve the authored foot orientation while the hip and knee flex.
        animation.animation[leg[2]].rotation *=
            glm::inverse(orientation(this->globals[leg[2]])) * orientation(ankle);
        this->update_globals(model, animation);
    }

    void CursorPose::crouch(const component::ModelComponent& model,
        component::AnimationComponent& animation,
        float weight) {
        if (!(weight > EPSILON) || !(this->legs[0][0] < this->globals.size()) ||
            !(this->legs[1][0] < this->globals.size())) {
            return;
        }
        std::array<glm::mat4, 2> ankles;
        float depth = 0;
        for (std::size_t side = 0; side < this->legs.size(); ++side) {
            const auto& leg = this->legs[side];
            ankles[side] = this->globals[leg[2]];
            const auto hip = glm::vec3(this->globals[leg[0]][3]);
            const auto knee = glm::vec3(this->globals[leg[1]][3]);
            if (!(hip.y > ankles[side][3].y)) {
                return;
            }
            const float length =
                glm::length(knee - hip) + glm::length(glm::vec3(ankles[side][3]) - knee);
            // For a nearly straight leg, shortening = L*(1-cos(knee_angle/2)).
            const float amount =
                length * (1.0f - std::cos(glm::radians(KNEE_BEND_DEGREES) * 0.5f)) * weight;
            depth = side == 0 ? amount : (std::min)(depth, amount);
        }
        for (const auto& leg : this->legs) {
            for (const auto bone : leg) {
                if (!(std::abs(glm::determinant(glm::mat3(this->globals[bone]))) > EPSILON)) {
                    return;
                }
            }
        }
        std::vector<glm::vec3> goals;
        for (const auto bone : this->leg_ik_goals) {
            goals.emplace_back(this->globals[bone][3]);
        }
        for (const auto root : this->roots) {
            if (!(std::abs(glm::determinant(glm::mat3(this->external[root]))) > EPSILON)) {
                return;
            }
        }
        for (const auto root : this->roots) {
            const auto axes = glm::mat3(this->external[root]);
            // delta_local = inverse(external) * delta_model.
            animation.animation[root].position += glm::inverse(axes) * glm::vec3(0, -depth, 0);
        }
        this->update_globals(model, animation);
        for (std::size_t side = 0; side < this->legs.size(); ++side) {
            this->bend_leg(model, animation, side, ankles[side]);
        }
        for (const auto bone : this->traversal) {
            const auto found = std::ranges::find(this->leg_ik_goals, bone);
            if (found == this->leg_ik_goals.end()) {
                continue;
            }
            const auto index = static_cast<std::size_t>(found - this->leg_ik_goals.begin());
            const auto parent = model.bone_node[bone].parent;
            const auto parent_matrix =
                parent < this->globals.size() ? this->globals[parent] : glm::mat4(1);
            // Keep downstream foot/toe IK goals planted: p_local = inverse(E*G_parent)*p_goal.
            animation.animation[bone].position = glm::vec3(
                glm::inverse(this->external[bone] * parent_matrix) * glm::vec4(goals[index], 1));
            this->update_globals(model, animation);
        }
    }
} // namespace enishi::animation
