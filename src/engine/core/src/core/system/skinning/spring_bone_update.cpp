#include "spring_bone_update.h"
#include <algorithm>
#include <cmath>
#include <glm/gtc/matrix_transform.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

namespace enishi::core {
    namespace {
        constexpr float DIRECTION_EPSILON = 0.000001f;
        constexpr float REFERENCE_STEP = 1.0f / 60.0f;
        constexpr float MAX_FRAME_TIME = 0.25f;

        glm::vec3 direction(glm::vec3 value, glm::vec3 fallback) {
            const auto length = glm::length(value);
            return length > DIRECTION_EPSILON ? value / length : fallback;
        }

        float scale(const glm::mat4& transform) {
            return std::max({glm::length(glm::vec3(transform[0])),
                glm::length(glm::vec3(transform[1])),
                glm::length(glm::vec3(transform[2]))});
        }

        glm::vec3 resolve_collisions(const types::SpringChain& chain,
            const component::ModelComponent& model,
            const std::vector<glm::mat4>& globals,
            glm::vec3 tail,
            glm::vec3 head,
            float length,
            float radius,
            glm::vec3 axis) {
            for (const auto index : chain.colliders) {
                const auto& collider = model.spring_bones.colliders[index];
                const auto& transform = globals[collider.bone];
                auto closest = glm::vec3(transform * glm::vec4(collider.offset, 1));
                if (collider.capsule) {
                    const auto end = glm::vec3(transform * glm::vec4(collider.tail, 1));
                    const auto segment = end - closest;
                    const auto squared = glm::dot(segment, segment);
                    if (squared > DIRECTION_EPSILON) {
                        // Closest point on a capsule segment: a + clamp(dot(p-a,b-a)/|b-a|²) *
                        // (b-a).
                        closest +=
                            std::clamp(glm::dot(tail - closest, segment) / squared, 0.0f, 1.0f) *
                            segment;
                    }
                }
                const auto separation = radius + collider.radius * scale(transform);
                const auto delta = tail - closest;
                if (glm::dot(delta, delta) < separation * separation) {
                    tail = closest + direction(delta, axis) * separation;
                    // Project back onto the fixed bone-length sphere after resolving penetration.
                    tail = head + direction(tail - head, axis) * length;
                }
            }
            return tail;
        }

        void rotate_subtree(const component::ModelComponent& model,
            std::vector<glm::mat4>& globals,
            types::BoneIndex bone,
            glm::vec3 head,
            glm::vec3 from,
            glm::vec3 to) {
            // Rotation around the head: M' = T(head) * R(from,to) * T(-head) * M.
            const auto correction = glm::translate(glm::mat4(1), head) *
                                    glm::mat4_cast(glm::rotation(from, to)) *
                                    glm::translate(glm::mat4(1), -head);
            std::vector<types::BoneIndex> pending{bone};
            for (std::size_t cursor = 0; cursor < pending.size(); ++cursor) {
                const auto index = pending[cursor];
                globals[index] = correction * globals[index];
                const auto& children = model.bone_node[index].children;
                pending.insert(pending.end(), children.begin(), children.end());
            }
        }

        void update_chain(const component::ModelComponent& model,
            std::vector<glm::mat4>& globals,
            const types::SpringChain& chain,
            std::vector<SpringTailState>& states,
            float delta) {
            const auto center =
                chain.center < globals.size() ? globals[chain.center] : glm::mat4(1);
            const auto inverse_center = glm::inverse(center);
            states.resize(chain.joints.size());
            for (std::size_t index = 0; index < chain.joints.size(); ++index) {
                const auto& joint = chain.joints[index];
                auto& state = states[index];
                const auto& transform = globals[joint.bone];
                const auto head = glm::vec3(transform[3]);
                const auto rest = glm::vec3(transform * glm::vec4(joint.local_tail, 1));
                const auto length = glm::length(rest - head);
                if (!(length > DIRECTION_EPSILON)) {
                    continue;
                }
                const auto axis = (rest - head) / length;
                if (!state.initialized) {
                    state.current = glm::vec3(inverse_center * glm::vec4(rest, 1));
                    state.previous = state.current;
                    state.previous_delta = delta;
                    state.initialized = true;
                }
                const auto current = glm::vec3(center * glm::vec4(state.current, 1));
                const auto previous = glm::vec3(center * glm::vec4(state.previous, 1));
                // Verlet: next = current + velocity * dt * damping + dt * (stiffness * axis +
                // gravity). Scale displacement by dt/previous_dt; normalize damping to the
                // reference 60 Hz step.
                const auto damping = std::pow(1.0f - joint.drag, delta / REFERENCE_STEP);
                const auto inertia =
                    (current - previous) * (delta / state.previous_delta) * damping;
                auto tail = current + inertia + delta * (joint.stiffness * axis + joint.gravity);
                tail = head + direction(tail - head, axis) * length;
                tail = resolve_collisions(chain,
                    model,
                    globals,
                    tail,
                    head,
                    length,
                    joint.radius * scale(transform),
                    axis);
                state.previous = state.current;
                state.current = glm::vec3(inverse_center * glm::vec4(tail, 1));
                state.previous_delta = delta;
                rotate_subtree(
                    model, globals, joint.bone, head, axis, direction(tail - head, axis));
            }
        }
    } // namespace

    void update_spring_bones(const component::ModelComponent& model,
        component::AnimationComponent& animation,
        SpringBoneState& state,
        float delta_seconds) {
        if (model.spring_bones.chains.empty() || !std::isfinite(delta_seconds) ||
            !(delta_seconds > 0)) {
            return;
        }
        if (delta_seconds > MAX_FRAME_TIME) {
            state.chains.clear();
            delta_seconds = REFERENCE_STEP;
        }
        state.chains.resize(model.spring_bones.chains.size());
        // Subdivide long frames so collision and length constraints run at least at 60 Hz.
        const auto steps = static_cast<int>(std::ceil(delta_seconds / REFERENCE_STEP));
        const auto delta = delta_seconds / static_cast<float>(steps);
        const auto base = animation.global;
        for (int step = 0; step < steps; ++step) {
            animation.global = base;
            for (std::size_t index = 0; index < state.chains.size(); ++index) {
                update_chain(model,
                    animation.global,
                    model.spring_bones.chains[index],
                    state.chains[index],
                    delta);
            }
        }
    }
} // namespace enishi::core
