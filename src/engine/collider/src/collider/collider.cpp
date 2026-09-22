#include "collider.h"
#include "obb_maker.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <iterator>
#include <limits>

namespace enishi::collider {
    namespace {
        bool finite(const glm::vec3& value) {
            return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
        }

        bool valid(const types::OBB& obb) {
            if (!finite(obb.center) || !finite(obb.half_extent)) {
                return false;
            }
            for (int i = 0; i < 3; ++i) {
                if (obb.half_extent[i] < 0.0f || !finite(obb.axis[i]) ||
                    std::abs(glm::dot(obb.axis[i], obb.axis[i]) - 1.0f) > 1e-5f) {
                    return false;
                }
                for (int j = 0; j < i; ++j) {
                    if (std::abs(glm::dot(obb.axis[i], obb.axis[j])) > 1e-5f) {
                        return false;
                    }
                }
            }
            return true;
        }

        double radius(const types::OBB& obb, const glm::dvec3& axis) {
            double result = 0.0;
            for (int i = 0; i < 3; ++i) {
                result += obb.half_extent[i] * std::abs(glm::dot(glm::dvec3(obb.axis[i]), axis));
            }
            return result;
        }

        // Intersect a line interval with the three pairs of OBB face planes.
        bool clip(const glm::dvec3& origin,
            const glm::dvec3& direction,
            const types::OBB& obb,
            double margin,
            double& lower,
            double& upper) {
            const glm::dvec3 offset = origin - glm::dvec3(obb.center);
            for (int i = 0; i < 3; ++i) {
                const glm::dvec3 axis(obb.axis[i]);
                const double start = glm::dot(offset, axis);
                const double speed = glm::dot(direction, axis);
                const double extent = obb.half_extent[i] + margin;
                if (speed == 0.0) {
                    if (std::abs(start) > extent) {
                        return false;
                    }
                    continue;
                }
                double near = (-extent - start) / speed;
                double far = (extent - start) / speed;
                if (near > far) {
                    std::swap(near, far);
                }
                lower = std::max(lower, near);
                upper = std::min(upper, far);
                if (lower > upper) {
                    return false;
                }
            }
            return true;
        }

        std::array<glm::dvec3, 8> corners(const types::OBB& obb) {
            std::array<glm::dvec3, 8> result;
            for (int bits = 0; bits < 8; ++bits) {
                result[bits] = glm::dvec3(obb.center);
                for (int axis = 0; axis < 3; ++axis) {
                    result[bits] += glm::dvec3(obb.axis[axis]) *
                                    static_cast<double>(obb.half_extent[axis]) *
                                    ((bits & (1 << axis)) ? 1.0 : -1.0);
                }
            }
            return result;
        }

        void accumulate_intersection(const types::OBB& source,
            const types::OBB& target,
            double margin,
            const glm::dvec3& reference,
            glm::dvec3& sum,
            std::size_t& count) {
            const auto points = corners(source);
            for (int bits = 0; bits < 8; ++bits) {
                for (int axis = 0; axis < 3; ++axis) {
                    if ((bits & (1 << axis)) != 0) {
                        continue;
                    }
                    const glm::dvec3 direction = points[bits | (1 << axis)] - points[bits];
                    double lower = 0.0;
                    double upper = 1.0;
                    if (clip(points[bits], direction, target, margin, lower, upper)) {
                        sum += (points[bits] - reference) + direction * lower;
                        sum += (points[bits] - reference) + direction * upper;
                        count += 2;
                    }
                }
            }
        }

        struct Bounds {
            glm::dvec3 lower{std::numeric_limits<double>::max()};
            glm::dvec3 upper{std::numeric_limits<double>::lowest()};
        };

        Bounds bounds(const Collider::OBBMap& map) {
            Bounds result;
            for (const auto& [bone, obb] : map) {
                glm::dvec3 extent(0.0);
                for (int i = 0; i < 3; ++i) {
                    extent +=
                        glm::abs(glm::dvec3(obb.axis[i])) * static_cast<double>(obb.half_extent[i]);
                }
                result.lower = glm::min(result.lower, glm::dvec3(obb.center) - extent);
                result.upper = glm::max(result.upper, glm::dvec3(obb.center) + extent);
            }
            return result;
        }

        bool overlaps(const Bounds& first, const Bounds& second) {
            for (int i = 0; i < 3; ++i) {
                if (first.upper[i] < second.lower[i] || second.upper[i] < first.lower[i]) {
                    return false;
                }
            }
            return true;
        }
    } // namespace

    bool Collider::register_model(types::CollisionModelId model) {
        return this->models.try_emplace(model).second;
    }

    bool Collider::remove_model(types::CollisionModelId model) {
        return this->models.erase(model) != 0;
    }

    bool Collider::set_bone(types::CollisionModelId model,
        types::BoneIndex bone,
        const std::vector<glm::vec3>& positions,
        const glm::mat4& transform) {
        const auto found = this->models.find(model);
        if (found == this->models.end() || bone == types::INVALID_BONE_INDEX) {
            return false;
        }
        const auto local = OBBMaker::make_by_covariance_matrix(positions);
        if (!local) {
            return false;
        }
        const auto world = OBBMaker::transform(*local, transform);
        if (!world) {
            return false;
        }
        found->second.local_obbs.insert_or_assign(bone, *local);
        found->second.world_obbs.insert_or_assign(bone, *world);
        return true;
    }

    bool Collider::update_bone(
        types::CollisionModelId model, types::BoneIndex bone, const glm::mat4& transform) {
        const auto found = this->models.find(model);
        if (found == this->models.end()) {
            return false;
        }
        const auto local = found->second.local_obbs.find(bone);
        if (local == found->second.local_obbs.end()) {
            return false;
        }
        const auto world = OBBMaker::transform(local->second, transform);
        if (!world) {
            return false;
        }
        found->second.world_obbs.insert_or_assign(bone, *world);
        return true;
    }

    bool Collider::remove_bone(types::CollisionModelId model, types::BoneIndex bone) {
        const auto found = this->models.find(model);
        if (found == this->models.end()) {
            return false;
        }
        found->second.local_obbs.erase(bone);
        return found->second.world_obbs.erase(bone) != 0;
    }

    const Collider::OBBMap* Collider::get_obb_map(types::CollisionModelId model) const noexcept {
        const auto found = this->models.find(model);
        return found == this->models.end() ? nullptr : &found->second.world_obbs;
    }

    bool Collider::add_handler(types::CollisionModelId model,
        const std::shared_ptr<sub_system::ICollisionHandler>& handler) {
        const auto found = this->models.find(model);
        if (found == this->models.end() || !handler) {
            return false;
        }
        auto& handlers = found->second.handlers;
        if (std::find(handlers.begin(), handlers.end(), handler) != handlers.end()) {
            return false;
        }
        handlers.push_back(handler);
        return true;
    }

    bool Collider::remove_handler(types::CollisionModelId model,
        const std::shared_ptr<sub_system::ICollisionHandler>& handler) {
        const auto found = this->models.find(model);
        if (found == this->models.end()) {
            return false;
        }
        return std::erase(found->second.handlers, handler) != 0;
    }

    std::size_t Collider::check_collisions(void) {
        if (this->models.size() < 2) {
            return 0;
        }
        struct Notification {
            std::shared_ptr<sub_system::ICollisionHandler> handler;
            types::BoneIndex bone;
            types::Collision collision;
        };
        std::vector<Notification> notifications;
        std::unordered_map<types::CollisionModelId, Bounds> model_bounds;
        for (const auto& [id, model] : this->models) {
            model_bounds.emplace(id, bounds(model.world_obbs));
        }
        std::size_t count = 0;
        for (auto first = this->models.begin(); first != this->models.end(); ++first) {
            // Each pair of distinct model instances is visited once; no self-bone pairs are formed.
            for (auto second = std::next(first); second != this->models.end(); ++second) {
                if (first->second.world_obbs.empty() || second->second.world_obbs.empty() ||
                    !overlaps(model_bounds.at(first->first), model_bounds.at(second->first))) {
                    continue;
                }
                for (const auto& [first_bone, first_obb] : first->second.world_obbs) {
                    for (const auto& [second_bone, second_obb] : second->second.world_obbs) {
                        const auto contact =
                            Collider::intersect(first->first, first_obb, second->first, second_obb);
                        if (!contact) {
                            continue;
                        }
                        ++count;
                        for (const auto& handler : first->second.handlers) {
                            notifications.push_back(
                                {handler, first_bone, {second->first, second_bone, *contact}});
                        }
                        auto reverse = *contact;
                        reverse.normal = -reverse.normal;
                        for (const auto& handler : second->second.handlers) {
                            notifications.push_back(
                                {handler, second_bone, {first->first, first_bone, reverse}});
                        }
                    }
                }
            }
        }
        // Hold handler ownership until the whole batch is delivered, even if a callback removes a
        // model.
        for (const auto& notification : notifications) {
            notification.handler->on_collision(notification.bone, notification.collision);
        }
        return count;
    }

    std::optional<types::OBBContact> Collider::intersect(types::CollisionModelId first_model,
        const types::OBB& first,
        types::CollisionModelId second_model,
        const types::OBB& second) {
        if (first_model == second_model || !valid(first) || !valid(second)) {
            return std::nullopt;
        }
        const glm::dvec3 delta = glm::dvec3(second.center) - glm::dvec3(first.center);
        double depth = std::numeric_limits<double>::max();
        glm::dvec3 normal(0.0);
        const auto test_axis = [&](glm::dvec3 axis) {
            const double length_squared = glm::dot(axis, axis);
            if (length_squared <= 1e-24) {
                return true;
            }
            axis /= std::sqrt(length_squared);
            const double distance = glm::dot(delta, axis);
            const double overlap = radius(first, axis) + radius(second, axis) - std::abs(distance);
            if (overlap < 0.0) {
                return false;
            }
            if (overlap < depth) {
                depth = overlap;
                normal = distance < 0.0 ? -axis : axis;
            }
            return true;
        };
        for (int i = 0; i < 3; ++i) {
            if (!test_axis(glm::dvec3(first.axis[i])) || !test_axis(glm::dvec3(second.axis[i]))) {
                return std::nullopt;
            }
        }
        for (int i = 0; i < 3; ++i) {
            for (int j = 0; j < 3; ++j) {
                if (!test_axis(glm::cross(glm::dvec3(first.axis[i]), glm::dvec3(second.axis[j])))) {
                    return std::nullopt;
                }
            }
        }

        // Clipped box edges include the intersection vertices, even for containment and edge/edge
        // hits. Their average lies in the convex intersection; it is not a mesh-surface contact
        // manifold.
        const glm::dvec3 reference(first.center);
        glm::dvec3 sum(0.0);
        std::size_t count = 0;
        accumulate_intersection(first, second, 0.0, reference, sum, count);
        accumulate_intersection(second, first, 0.0, reference, sum, count);
        if (count == 0) {
            // Float axes need a small relative allowance when touching faces are nearly coplanar.
            const double scale = std::max({static_cast<double>(first.half_extent.x),
                static_cast<double>(first.half_extent.y),
                static_cast<double>(first.half_extent.z),
                static_cast<double>(second.half_extent.x),
                static_cast<double>(second.half_extent.y),
                static_cast<double>(second.half_extent.z)});
            accumulate_intersection(first, second, scale * 1e-6, reference, sum, count);
            accumulate_intersection(second, first, scale * 1e-6, reference, sum, count);
        }
        if (count == 0 || depth > std::numeric_limits<float>::max()) {
            return std::nullopt;
        }
        return types::OBBContact{glm::vec3(reference + sum / static_cast<double>(count)),
            glm::vec3(normal),
            static_cast<float>(depth)};
    }

    bool Collider::hit_model(const types::Ray& ray, const types::OBB& obb) {
        if (!valid(obb) || !finite(ray.origin) || !finite(ray.direction) ||
            glm::dot(glm::dvec3(ray.direction), glm::dvec3(ray.direction)) == 0.0) {
            return false;
        }
        double lower = 0.0;
        double upper = std::numeric_limits<double>::max();
        return clip(glm::dvec3(ray.origin), glm::dvec3(ray.direction), obb, 0.0, lower, upper);
    }
} // namespace enishi::collider
