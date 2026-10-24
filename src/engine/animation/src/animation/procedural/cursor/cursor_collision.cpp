#include "cursor_collision.h"
#include <algorithm>
#include <animation/procedural/cursor/cursor_reach.h>
#include <cmath>
#include <collider/collider.h>
#include <collider/obb_maker.h>
#include <debug/log/logger.h>

namespace enishi::animation {
    using enum CursorPoseState::Joint;
    using enum CursorPoseState::BodyBone;
    namespace {
        bool belongs_to(
            const component::ModelComponent& model, types::BoneIndex bone, types::BoneIndex root) {
            if (!(root < model.bone_node.size())) {
                return false;
            }
            for (std::size_t step = 0; step < model.bone_node.size(); ++step) {
                if (!(bone < model.bone_node.size())) {
                    return false;
                }
                if (bone == root) {
                    return true;
                }
                bone = model.bone_node[bone].parent;
            }
            return false;
        }

        bool adjacent(const component::ModelComponent& model,
            types::BoneIndex first,
            types::BoneIndex second) {
            return first < model.bone_node.size() && second < model.bone_node.size() &&
                   (model.bone_node[first].parent == second ||
                       model.bone_node[second].parent == first);
        }

        bool aabb_overlaps(const glm::vec3& first_low,
            const glm::vec3& first_high,
            const glm::vec3& second_low,
            const glm::vec3& second_high) {
            return !glm::any(glm::greaterThan(first_low, second_high)) &&
                   !glm::any(glm::greaterThan(second_low, first_high));
        }

        bool segment_hits(
            const glm::vec3& start, const glm::vec3& end, const types::OBB& box, float margin) {
            float near = 0;
            float far = 1;
            constexpr float EPSILON = 0.00001f;
            // Slab clipping in box coordinates: -extent <= p+t*d <= extent, t in [0,1].
            // Inflating each extent by the capsule radius conservatively encloses its sweep.
            for (int axis = 0; axis < 3; ++axis) {
                const float p = glm::dot(start - box.center, box.axis[axis]);
                const float d = glm::dot(end - start, box.axis[axis]);
                const float extent = box.half_extent[axis] + margin;
                if (std::abs(d) < EPSILON) {
                    if (std::abs(p) > extent) {
                        return false;
                    }
                    continue;
                }
                float first = (-extent - p) / d;
                float last = (extent - p) / d;
                if (first > last) {
                    std::swap(first, last);
                }
                near = (std::max)(near, first);
                far = (std::min)(far, last);
                if (near > far) {
                    return false;
                }
            }
            return true;
        }
    } // namespace

    void CursorCollision::initialize_collision(
        CursorPoseState& state, const component::ModelComponent& model) {
        for (types::BoneIndex bone = 0; bone < model.collision_vertices.size(); ++bone) {
            if (!(bone < model.bone_node.size()) || model.collision_vertices[bone].empty()) {
                continue;
            }
            const auto local =
                collider::OBBMaker::make_by_covariance_matrix(model.collision_vertices[bone]);
            if (local.is_none()) {
                debug::Logger::warning("Cannot build cursor collision bounds");
                continue;
            }
            CursorCollisionBone bounds{bone, local.unwrap()};
            for (std::size_t side = 0; side < state.wrists.size(); ++side) {
                if (belongs_to(model, bone, state.joints[side == 0 ? LeftArm : RightArm])) {
                    bounds.arm = static_cast<int>(side);
                    bounds.distal =
                        belongs_to(model, bone, state.joints[side == 0 ? LeftElbow : RightElbow]);
                    bounds.hand = belongs_to(model, bone, state.wrists[side]);
                }
            }
            for (const auto& body : model.rigid_bodies) {
                if (body.kind != types::RigidBodyKind::Kinematic &&
                    belongs_to(model, bone, body.relate_bone_index)) {
                    bounds.physics = true;
                    break;
                }
            }
            state.collision.bones.push_back(std::move(bounds));
        }
        CursorCollision::initialize_collision_queries(state, model);
    }

    float CursorCollision::hand_radius(const CursorPoseState& state, std::size_t side) {
        float radius = CursorReach::arm_frame(state, side).hand_margin;
        const auto wrist = glm::vec3(state.transforms.globals[state.wrists[side]][3]);
        for (const auto index : state.collision.hand_bounds[side]) {
            const auto& bounds = CursorCollision::collision_bound(state, index);
            if (!bounds.world.has_value()) {
                continue;
            }
            // |center-wrist| + |half_extent| encloses all eight OBB corners about the wrist.
            radius = (std::max)(radius,
                glm::length(bounds.world->center - wrist) + glm::length(bounds.world->half_extent));
        }
        return radius;
    }

    bool CursorCollision::mesh_arm_clear(const CursorPoseState& state, std::size_t side) {
        if (!state.config.experimental_obb_collision) {
            return true;
        }
        const auto elbow = state.joints[side == 0 ? LeftElbow : RightElbow];
        const auto start = glm::vec3(state.transforms.globals[elbow][3]);
        const auto end = glm::vec3(state.transforms.globals[state.wrists[side]][3]);
        const auto frame = CursorReach::arm_frame(state, side);
        const auto limb_low = (glm::min)(start, end) - glm::vec3(frame.arm_margin);
        const auto limb_high = (glm::max)(start, end) + glm::vec3(frame.arm_margin);
        const auto hand_low = end - glm::vec3(frame.hand_margin);
        const auto hand_high = end + glm::vec3(frame.hand_margin);
        for (const auto& query : state.collision.queries[side]) {
            const auto& obstacle = CursorCollision::collision_bound(state, query.obstacle);
            if (!obstacle.world.has_value()) {
                continue;
            }
            // Upper arms attach to the torso, so their shared shoulder region is not tested
            // against clothing. Forearms, hands and fingers must clear it in their entirety.
            if ((query.forearm &&
                    aabb_overlaps(limb_low, limb_high, obstacle.lower, obstacle.upper) &&
                    segment_hits(start, end, *obstacle.world, frame.arm_margin)) ||
                (query.hand && aabb_overlaps(hand_low, hand_high, obstacle.lower, obstacle.upper) &&
                    segment_hits(end, end, *obstacle.world, frame.hand_margin))) {
                return false;
            }
            for (const auto index : query.limbs) {
                const auto& limb = CursorCollision::collision_bound(state, index);
                if (limb.world.has_value() &&
                    aabb_overlaps(limb.lower, limb.upper, obstacle.lower, obstacle.upper)) {
                    if (collider::Collider::overlap(*limb.world, *obstacle.world)) {
                        return false;
                    }
                }
            }
        }
        return true;
    }

    void CursorCollision::initialize_collision_queries(
        CursorPoseState& state, const component::ModelComponent& model) {
        for (std::size_t side = 0; side < state.wrists.size(); ++side) {
            const int owner = static_cast<int>(side);
            const auto elbow = state.joints[side == 0 ? LeftElbow : RightElbow];
            for (std::size_t index = 0; index < state.collision.bones.size(); ++index) {
                const auto& bounds = state.collision.bones[index];
                if (bounds.arm == owner) {
                    if (bounds.hand) {
                        state.collision.hand_bounds[side].push_back(index);
                    }
                    continue;
                }
                CursorCollisionQuery query{{},
                    index,
                    !adjacent(model, elbow, bounds.bone),
                    !adjacent(model, state.wrists[side], bounds.bone)};
                for (std::size_t limb = 0; limb < state.collision.bones.size(); ++limb) {
                    const auto& candidate = state.collision.bones[limb];
                    if (candidate.arm == owner && candidate.distal &&
                        !adjacent(model, candidate.bone, bounds.bone)) {
                        query.limbs.push_back(limb);
                    }
                }
                state.collision.queries[side].push_back(std::move(query));
            }
        }
    }

    const CursorCollisionBone& CursorCollision::collision_bound(
        const CursorPoseState& state, std::size_t index) {
        auto& bounds = state.collision.bones[index];
        const auto& matrix = bounds.physics && bounds.bone < state.collision.physics_globals.size()
                                 ? state.collision.physics_globals[bounds.bone]
                                 : state.transforms.globals[bounds.bone];
        if (bounds.transform == matrix) {
            return bounds;
        }
        bounds.transform = matrix;
        const auto transformed = collider::OBBMaker::transform(bounds.local, matrix);
        if (transformed.is_some()) {
            bounds.world = transformed.unwrap();
            // World AABB half-size e=sum_i |axis_i|*half_extent_i encloses all OBB corners.
            glm::vec3 extent(0);
            for (int axis = 0; axis < 3; ++axis) {
                extent += glm::abs(bounds.world->axis[axis]) * bounds.world->half_extent[axis];
            }
            bounds.lower = bounds.world->center - extent;
            bounds.upper = bounds.world->center + extent;
        } else {
            bounds.world.reset();
            debug::Logger::warning("Cannot transform cursor collision bounds");
        }
        return bounds;
    }
} // namespace enishi::animation
