#include "vrm_spring.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <set>

namespace enishi::assets_system {
    namespace {
        glm::vec3 vector(const VRMJson& value, bool legacy, glm::vec3 fallback = glm::vec3(0)) {
            if (!value.valid) {
                return fallback;
            }
            if (legacy) {
                return {value["x"].number(), value["y"].number(), value["z"].number()};
            }
            if (value.elements.size() != 3) {
                return glm::vec3(std::numeric_limits<float>::quiet_NaN());
            }
            return {
                value.elements[0].number(), value.elements[1].number(), value.elements[2].number()};
        }

        bool finite(glm::vec3 value) {
            return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
        }

        bool add_collider(VRMContext& context,
            types::AddonSpringBones& springs,
            std::size_t node,
            const VRMJson& shape,
            bool capsule) {
            types::SpringCollider collider;
            collider.bone = node;
            auto offset = vector(shape["offset"], context.legacy);
            if (context.legacy) {
                // VRM 0 collider offsets use the opposite Z axis; gravity is already model-space.
                offset.z = -offset.z;
            }
            collider.offset = glm::mat3(context.basis) * offset;
            collider.tail = glm::mat3(context.basis) * vector(shape["tail"], context.legacy);
            collider.radius = shape["radius"].number() * VRM_MODEL_UNITS_PER_METER;
            collider.capsule = capsule;
            if (!(node < context.source.nodes_count) || !finite(collider.offset) ||
                !finite(collider.tail) || !std::isfinite(collider.radius) || collider.radius < 0) {
                return context.fail("Invalid VRM spring collider");
            }
            springs.colliders.push_back(collider);
            return true;
        }

        bool read_colliders(VRMContext& context,
            const VRMJson& source,
            types::AddonSpringBones& springs,
            std::vector<std::vector<std::size_t>>& groups) {
            if (!context.legacy) {
                for (const auto& collider : source["colliders"].elements) {
                    const auto& shape = collider["shape"];
                    const bool capsule = shape["capsule"].valid;
                    if (!add_collider(context,
                            springs,
                            collider["node"].index(),
                            shape[capsule ? "capsule" : "sphere"],
                            capsule)) {
                        return false;
                    }
                }
            }
            for (const auto& group : source["colliderGroups"].elements) {
                std::vector<std::size_t> indices;
                for (const auto& collider : group["colliders"].elements) {
                    if (context.legacy) {
                        indices.push_back(springs.colliders.size());
                        if (!add_collider(
                                context, springs, group["node"].index(), collider, false)) {
                            return false;
                        }
                    } else {
                        const auto index = collider.index();
                        if (!(index < springs.colliders.size())) {
                            return context.fail("Invalid VRM collider group reference");
                        }
                        indices.push_back(index);
                    }
                }
                groups.push_back(std::move(indices));
            }
            return true;
        }

        bool add_joint(VRMContext& context,
            types::SpringChain& chain,
            const VRMJson& source,
            std::size_t node,
            std::size_t tail) {
            if (!(node < context.source.nodes_count)) {
                return context.fail("Invalid VRM spring joint node");
            }
            const auto& bones = std::get<types::AddonBones>(context.model.addons.front());
            const auto& bone = bones[node];
            types::SpringJoint joint;
            joint.bone = node;
            if (tail < bones.size()) {
                auto parent = bones[tail].bone_node.parent;
                while (parent != node && parent < bones.size()) {
                    parent = bones[parent].bone_node.parent;
                }
                if (parent != node) {
                    return context.fail("VRM spring joints must form an ancestor chain");
                }
                // Tail in head-local space = inverse(head bind) * tail bind position.
                joint.local_tail = glm::vec3(
                    glm::inverse(bone.bind_bone.global) * bones[tail].bind_bone.global[3]);
            } else {
                constexpr float LEGACY_TAIL_METERS = 0.07f;
                const auto parent = bone.bone_node.parent;
                const auto direction =
                    parent < bones.size()
                        ? glm::vec3(bone.bind_bone.global[3] - bones[parent].bind_bone.global[3])
                        : glm::vec3(0, 1, 0);
                if (!(glm::dot(direction, direction) > 0)) {
                    return context.fail("Zero length VRM spring terminal bone");
                }
                joint.local_tail = glm::mat3(glm::inverse(bone.bind_bone.global)) *
                                   glm::normalize(direction) * LEGACY_TAIL_METERS *
                                   VRM_MODEL_UNITS_PER_METER;
            }
            joint.stiffness = source[context.legacy ? "stiffiness" : "stiffness"].number(1) *
                              VRM_MODEL_UNITS_PER_METER;
            joint.drag = source["dragForce"].number(0.4f);
            joint.radius = source["hitRadius"].number() * VRM_MODEL_UNITS_PER_METER;
            joint.gravity = glm::mat3(context.basis) *
                            vector(source["gravityDir"], context.legacy, glm::vec3(0, -1, 0)) *
                            source["gravityPower"].number();
            if (!finite(joint.local_tail) || !(glm::dot(joint.local_tail, joint.local_tail) > 0) ||
                !finite(joint.gravity) || !std::isfinite(joint.stiffness) || joint.stiffness < 0 ||
                !std::isfinite(joint.drag) || joint.drag < 0 || joint.drag > 1 ||
                !std::isfinite(joint.radius) || joint.radius < 0) {
                return context.fail("Invalid VRM spring parameters");
            }
            chain.joints.push_back(joint);
            return true;
        }

        bool read_chain(VRMContext& context,
            const VRMJson& source,
            const std::vector<std::vector<std::size_t>>& groups,
            types::SpringChain& chain) {
            if (source["center"].valid && source["center"].value != "-1") {
                chain.center = source["center"].index();
                if (!(chain.center < context.source.nodes_count)) {
                    return context.fail("Invalid VRM spring center");
                }
            }
            for (const auto& group : source["colliderGroups"].elements) {
                const auto index = group.index();
                if (!(index < groups.size())) {
                    return context.fail("Invalid VRM spring collider group");
                }
                chain.colliders.insert(
                    chain.colliders.end(), groups[index].begin(), groups[index].end());
            }
            if (context.legacy) {
                std::vector<std::size_t> pending;
                for (const auto& root : source["bones"].elements) {
                    pending.push_back(root.index());
                }
                std::set<std::size_t> visited;
                for (std::size_t index = 0; index < pending.size(); ++index) {
                    const auto node_index = pending[index];
                    if (!(node_index < context.source.nodes_count) ||
                        !visited.insert(node_index).second) {
                        return context.fail("Invalid or duplicate VRM spring root");
                    }
                    const auto& node = context.source.nodes[node_index];
                    const auto tail =
                        node.children_count > 0
                            ? static_cast<std::size_t>(node.children[0] - context.source.nodes)
                            : types::INVALID_BONE_INDEX;
                    if (!add_joint(context, chain, source, node_index, tail)) {
                        return false;
                    }
                    for (std::size_t child = 0; child < node.children_count; ++child) {
                        pending.push_back(
                            static_cast<std::size_t>(node.children[child] - context.source.nodes));
                    }
                }
            } else {
                const auto& joints = source["joints"].elements;
                for (std::size_t index = 0; index + 1 < joints.size(); ++index) {
                    if (!add_joint(context,
                            chain,
                            joints[index],
                            joints[index]["node"].index(),
                            joints[index + 1]["node"].index())) {
                        return false;
                    }
                }
            }
            return true;
        }
    } // namespace

    bool make_vrm_springs(VRMContext& context) {
        auto source = context.legacy ? context.extension["secondaryAnimation"]
                                     : vrm_extension(context.source.data_extensions,
                                           context.source.data_extensions_count,
                                           "VRMC_springBone");
        if (!source.valid) {
            return true;
        }
        if (!context.legacy && source["specVersion"].value != "1.0") {
            return context.fail("Unsupported VRM spring specification version");
        }
        types::AddonSpringBones springs;
        std::vector<std::vector<std::size_t>> groups;
        if (!read_colliders(context, source, springs, groups)) {
            return false;
        }
        std::set<std::size_t> assigned;
        for (const auto& definition : source[context.legacy ? "boneGroups" : "springs"].elements) {
            types::SpringChain chain;
            if (!read_chain(context, definition, groups, chain)) {
                return false;
            }
            for (const auto& joint : chain.joints) {
                if (!assigned.insert(joint.bone).second) {
                    return context.fail("VRM spring chains share a joint");
                }
            }
            springs.chains.push_back(std::move(chain));
        }
        const auto depth = [&](const types::SpringChain& chain) {
            std::size_t result{};
            if (!chain.joints.empty()) {
                const auto* parent = context.source.nodes[chain.joints.front().bone].parent;
                while (parent != nullptr) {
                    ++result;
                    parent = parent->parent;
                }
            }
            return result;
        };
        std::stable_sort(springs.chains.begin(),
            springs.chains.end(),
            [&](const auto& left, const auto& right) { return depth(left) < depth(right); });
        context.model.addons.emplace_back(std::move(springs));
        return true;
    }
} // namespace enishi::assets_system
