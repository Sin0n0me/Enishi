#include "ik_solver.h"
#include <algorithm>
#include <cmath>

namespace enishi::ik {
    namespace {
        constexpr float DIRECTION_EPSILON = 1e-6f;
        constexpr float DISTANCE_EPSILON = 1e-5f;

        glm::vec3 local_direction(const glm::mat4& inverse, const glm::mat4& transform) {
            return glm::vec3(inverse * glm::vec4(glm::vec3(transform[3]), 1.0f));
        }

        glm::quat base_rotation(std::span<const glm::mat4> local, types::BoneIndex index) {
            if (!(index < local.size())) {
                return glm::quat(1, 0, 0, 0);
            }
            glm::mat3 rotation(local[index]);
            for (glm::length_t axis = 0; axis < rotation.length(); ++axis) {
                const auto length = glm::length(rotation[axis]);
                if (!(length > DIRECTION_EPSILON)) {
                    return glm::quat(1, 0, 0, 0);
                }
                rotation[axis] /= length;
            }
            return glm::normalize(glm::quat_cast(rotation));
        }

        glm::vec3 nearest_angles(glm::quat rotation, glm::vec3 previous) {
            auto angles = glm::eulerAngles(rotation);
            // Both Euler representations describe the same rotation. Keep the branch
            // nearest the previous iteration, including across the +/- pi boundary.
            auto alternate = glm::vec3(angles.x + glm::pi<float>(),
                glm::pi<float>() - angles.y,
                angles.z + glm::pi<float>());
            for (glm::length_t axis = 0; axis < angles.length(); ++axis) {
                angles[axis] = previous[axis] +
                               std::remainder(angles[axis] - previous[axis], glm::two_pi<float>());
                alternate[axis] = previous[axis] + std::remainder(alternate[axis] - previous[axis],
                                                       glm::two_pi<float>());
            }
            return glm::length(alternate - previous) < glm::length(angles - previous) ? alternate
                                                                                      : angles;
        }

        const types::IKLinkLimit* link_limit(const types::CCDIK& ik, std::size_t link) {
            if (link < ik.link_limits.size() && ik.link_limits[link].enabled) {
                return &ik.link_limits[link];
            }
            return nullptr;
        }

        glm::vec3 hinge_axis(const types::IKLinkLimit* limit) {
            glm::vec3 axis(0);
            if (limit == nullptr) {
                return axis;
            }
            int count = 0;
            for (glm::length_t component = 0; component < axis.length(); ++component) {
                if (limit->lower[component] != 0 || limit->upper[component] != 0) {
                    axis[component] = 1;
                    ++count;
                }
            }
            return count == 1 ? axis : glm::vec3(0);
        }

        std::vector<glm::vec3> initialize_limits(sub_system::IIKBoneViewList& views,
            const types::CCDIK& ik,
            std::span<const glm::mat4> base_local) {
            std::vector<glm::vec3> angles(ik.chain.size(), glm::vec3(0));
            for (std::size_t link = 0; link < ik.chain.size(); ++link) {
                const auto bone = views.get(ik.chain[link]);
                const auto* limit = link_limit(ik, link);
                if (bone.is_some() && limit != nullptr) {
                    const auto base = base_rotation(base_local, ik.chain[link]);
                    const auto center = limit->lower * 0.5f + limit->upper * 0.5f;
                    angles[link] =
                        glm::clamp(nearest_angles(base * bone.unwrap()->get_ik_rotation(), center),
                            limit->lower,
                            limit->upper);
                    bone.unwrap()->set_ik_rotation(
                        glm::normalize(glm::inverse(base) * glm::quat(angles[link])));
                }
            }
            return angles;
        }
    } // namespace

    void IKSolver::apply_ik(const types::IK& ik,
        sub_system::IIKBoneViewList* ik_view_list,
        sub_system::IBoneUpdater* updater,
        types::BoneIndex index,
        std::span<const glm::mat4> base_local) {
        if (ik_view_list == nullptr || updater == nullptr || !(index < ik_view_list->size())) {
            return;
        }
        const auto* ccd = std::get_if<types::CCDIK>(&ik.method);
        if (ccd != nullptr) {
            IKSolver::ccd_ik(*ik_view_list, *updater, *ccd, base_local);
        }
    }

    void IKSolver::ccd_ik(sub_system::IIKBoneViewList& views,
        sub_system::IBoneUpdater& updater,
        const types::CCDIK& ik,
        std::span<const glm::mat4> base_local) {
        if (ik.chain.empty() || ik.iterations == 0) {
            return;
        }
        const auto goal = views.get(ik.ik_bone);
        const auto target = views.get(ik.target);
        if (goal.is_none() || target.is_none()) {
            return;
        }
        // The goal must remain fixed while chain updates propagate through descendants.
        const auto goal_transform = goal.unwrap()->get_ik_global_transform();
        const auto distance = [&]() {
            return glm::length(
                glm::vec3(goal_transform[3] - target.unwrap()->get_ik_global_transform()[3]));
        };
        std::vector<glm::quat> previous(ik.chain.size());
        auto angles = initialize_limits(views, ik, base_local);
        updater.update_global_form_roots();
        for (std::uint32_t iteration = 0; iteration < ik.iterations; ++iteration) {
            const auto before = distance();
            if (before < DISTANCE_EPSILON) {
                break;
            }
            for (std::size_t link = 0; link < ik.chain.size(); ++link) {
                const auto bone = views.get(ik.chain[link]);
                if (bone.is_some()) {
                    previous[link] = bone.unwrap()->get_ik_rotation();
                }
            }
            for (std::size_t link = 0; link < ik.chain.size(); ++link) {
                const auto index = ik.chain[link];
                const auto bone = views.get(index);
                if (bone.is_none()) {
                    continue;
                }
                const auto inverse = glm::inverse(bone.unwrap()->get_ik_global_transform());
                const auto from =
                    local_direction(inverse, target.unwrap()->get_ik_global_transform());
                const auto to = local_direction(inverse, goal_transform);
                const auto base = base_rotation(base_local, index);
                const auto* bounds = link_limit(ik, link);
                const auto hinge = hinge_axis(bounds);
                glm::quat delta;
                if (glm::dot(hinge, hinge) > 0.0f) {
                    const auto step =
                        std::visit([](const auto& limit) { return limit.limit; }, ik.limit);
                    const auto axis = glm::inverse(base * bone.unwrap()->get_ik_rotation()) * hinge;
                    delta = IKSolver::axis_rotation(from, to, axis, step);
                } else if (const auto* limit = std::get_if<types::IKLimitAxis>(&ik.limit);
                           limit != nullptr) {
                    delta = IKSolver::axis_rotation(from, to, limit->axis, limit->limit);
                } else {
                    delta =
                        IKSolver::rotation(from, to, std::get<types::IKLimitAngle>(ik.limit).limit);
                }
                auto correction = glm::normalize(bone.unwrap()->get_ik_rotation() * delta);
                if (bounds != nullptr) {
                    angles[link] = glm::clamp(nearest_angles(base * correction, angles[link]),
                        bounds->lower,
                        bounds->upper);
                    correction = glm::normalize(glm::inverse(base) * glm::quat(angles[link]));
                }
                bone.unwrap()->set_ik_rotation(correction);
                updater.update_local(index);
                updater.update_global(index);
            }
            const auto after = distance();
            if (after > before + DISTANCE_EPSILON) {
                for (std::size_t link = 0; link < ik.chain.size(); ++link) {
                    const auto bone = views.get(ik.chain[link]);
                    if (bone.is_some()) {
                        bone.unwrap()->set_ik_rotation(previous[link]);
                    }
                }
                updater.update_global_form_roots();
                break;
            }
            if (std::abs(before - after) < DISTANCE_EPSILON) {
                break;
            }
        }
    }

    glm::quat IKSolver::rotation(glm::vec3 from, glm::vec3 to, float limit) {
        const glm::quat identity(1, 0, 0, 0);
        if (!(glm::length(from) > DIRECTION_EPSILON) || !(glm::length(to) > DIRECTION_EPSILON) ||
            !(limit > 0.0f)) {
            return identity;
        }
        from = glm::normalize(from);
        to = glm::normalize(to);
        const auto cosine = std::clamp(glm::dot(from, to), -1.0f, 1.0f);
        const auto angle = std::min(std::acos(cosine), limit);
        if (angle < DIRECTION_EPSILON) {
            return identity;
        }
        auto axis = glm::cross(from, to);
        if (glm::length(axis) < DIRECTION_EPSILON) {
            // Opposite directions have no unique axis; choose a stable perpendicular.
            const auto basis =
                std::abs(from.x) < std::abs(from.z) ? glm::vec3(1, 0, 0) : glm::vec3(0, 0, 1);
            axis = glm::cross(from, basis);
        }
        return glm::angleAxis(angle, glm::normalize(axis));
    }

    glm::quat IKSolver::axis_rotation(glm::vec3 from, glm::vec3 to, glm::vec3 axis, float limit) {
        const glm::quat identity(1, 0, 0, 0);
        if (!(glm::length(axis) > DIRECTION_EPSILON) || !(limit > 0.0f)) {
            return identity;
        }
        axis = glm::normalize(axis);
        from -= axis * glm::dot(from, axis);
        to -= axis * glm::dot(to, axis);
        if (!(glm::length(from) > DIRECTION_EPSILON) || !(glm::length(to) > DIRECTION_EPSILON)) {
            return identity;
        }
        from = glm::normalize(from);
        to = glm::normalize(to);
        const auto angle = std::atan2(glm::dot(axis, glm::cross(from, to)), glm::dot(from, to));
        return glm::angleAxis(std::clamp(angle, -limit, limit), axis);
    }
} // namespace enishi::ik
