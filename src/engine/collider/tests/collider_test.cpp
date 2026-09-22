#include <array>
#include <cmath>
#include <collider/collider.h>
#include <collider/obb_maker.h>
#include <functional>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <limits>
#include <random>

namespace {
    using enishi::collider::Collider;
    using enishi::collider::OBBMaker;
    namespace types = enishi::types;

    int failures = 0;
#define CHECK(expression)                                                                          \
    do {                                                                                           \
        if (!(expression)) {                                                                       \
            std::cerr << __LINE__ << ": " << #expression << '\n';                                  \
            ++failures;                                                                            \
        }                                                                                          \
    } while (false)

    bool near(float first, float second, float tolerance = 1e-4f) {
        return std::abs(first - second) <= tolerance;
    }

    types::OBB box(glm::vec3 center = glm::vec3(0),
        glm::vec3 extent = glm::vec3(1),
        const glm::mat3& rotation = glm::mat3(1)) {
        return {center, {rotation[0], rotation[1], rotation[2]}, extent};
    }

    std::vector<glm::vec3> vertices(const types::OBB& obb) {
        std::vector<glm::vec3> result;
        for (int bits = 0; bits < 8; ++bits) {
            glm::vec3 point = obb.center;
            for (int i = 0; i < 3; ++i) {
                point += obb.axis[i] * obb.half_extent[i] * ((bits & (1 << i)) ? 1.0f : -1.0f);
            }
            result.push_back(point);
        }
        return result;
    }

    bool contains(const types::OBB& obb, const glm::vec3& point, double tolerance = 1e-4) {
        for (int i = 0; i < 3; ++i) {
            if (std::abs(glm::dot(glm::dvec3(point) - glm::dvec3(obb.center),
                    glm::dvec3(obb.axis[i]))) > obb.half_extent[i] + tolerance) {
                return false;
            }
        }
        return true;
    }

    // Independent reference: enumerate vertices of the intersection of twelve half-spaces.
    bool reference_intersection(const types::OBB& first, const types::OBB& second) {
        std::array<glm::dvec3, 12> normals;
        std::array<double, 12> offsets;
        int index = 0;
        for (const auto& obb : {first, second}) {
            for (int i = 0; i < 3; ++i) {
                for (double sign : {-1.0, 1.0}) {
                    normals[index] = glm::dvec3(obb.axis[i]) * sign;
                    offsets[index] =
                        glm::dot(normals[index], glm::dvec3(obb.center)) + obb.half_extent[i];
                    ++index;
                }
            }
        }
        for (int i = 0; i < 12; ++i) {
            for (int j = i + 1; j < 12; ++j) {
                for (int k = j + 1; k < 12; ++k) {
                    const glm::dmat3 planes =
                        glm::transpose(glm::dmat3(normals[i], normals[j], normals[k]));
                    if (std::abs(glm::determinant(planes)) < 1e-10) {
                        continue;
                    }
                    const glm::dvec3 point =
                        glm::inverse(planes) * glm::dvec3(offsets[i], offsets[j], offsets[k]);
                    bool inside = true;
                    for (int p = 0; p < 12; ++p) {
                        if (glm::dot(normals[p], point) > offsets[p] + 1e-8) {
                            inside = false;
                            break;
                        }
                    }
                    if (inside) {
                        return true;
                    }
                }
            }
        }
        return false;
    }

    void test_maker() {
        CHECK(!OBBMaker::make_by_covariance_matrix({}));
        CHECK(!OBBMaker::make_by_covariance_matrix(
            {glm::vec3(std::numeric_limits<float>::quiet_NaN())}));
        CHECK(!OBBMaker::make_by_covariance_matrix(
            {glm::vec3(std::numeric_limits<float>::infinity())}));
        const glm::mat4 rotation =
            glm::rotate(glm::mat4(1), 0.7f, glm::normalize(glm::vec3(1, 2, 3)));
        const auto input =
            vertices(box(glm::vec3(5, -2, 8), glm::vec3(3, 2, 1), glm::mat3(rotation)));
        const auto obb = OBBMaker::make_by_covariance_matrix(input);
        CHECK(obb);
        if (!obb) {
            return;
        }
        for (const auto& point : input) {
            CHECK(contains(*obb, point));
        }
        CHECK(near(obb->half_extent.x * obb->half_extent.y * obb->half_extent.z, 6.0f));
        for (const auto& points : std::vector<std::vector<glm::vec3>>{{glm::vec3(3, 4, 5)},
                 {glm::vec3(0), glm::vec3(1), glm::vec3(2)},
                 {{-1, -1, 0}, {1, -1, 0}, {1, 1, 0}, {-1, 1, 0}}}) {
            const auto degenerate = OBBMaker::make_by_covariance_matrix(points);
            CHECK(degenerate);
            if (degenerate) {
                for (const auto& point : points) {
                    CHECK(contains(*degenerate, point));
                }
                CHECK(near(glm::length(degenerate->axis[0]), 1));
                CHECK(near(glm::dot(degenerate->axis[0], degenerate->axis[1]), 0));
            }
        }
        const auto rotated_cube = OBBMaker::transform(box(), rotation);
        CHECK(rotated_cube);
        if (rotated_cube) {
            CHECK(near(rotated_cube->half_extent.x * rotated_cube->half_extent.y *
                           rotated_cube->half_extent.z,
                1.0f));
        }
        glm::mat4 transform = glm::translate(glm::mat4(1), glm::vec3(7, 3, -1)) * rotation *
                              glm::scale(glm::mat4(1), glm::vec3(-2, 3, 0.5));
        transform[1][0] += 0.8f;
        const auto world = OBBMaker::transform(*obb, transform);
        CHECK(world);
        if (world) {
            for (const auto& point : vertices(*obb)) {
                CHECK(contains(*world, glm::vec3(transform * glm::vec4(point, 1))));
            }
        }
        CHECK(OBBMaker::transform(*obb, glm::scale(glm::mat4(1), glm::vec3(0))));
        transform[0][3] = 1;
        CHECK(!OBBMaker::transform(*obb, transform));
    }

    void test_intersection() {
        CHECK(!Collider::intersect(1, box(), 1, box()));
        CHECK(!Collider::intersect(1, box(), 2, box(glm::vec3(3, 0, 0))));
        for (const auto& other : {box(glm::vec3(1.5, 0, 0)),
                 box(glm::vec3(2, 0, 0)),
                 box(glm::vec3(2)),
                 box(glm::vec3(0), glm::vec3(0.25f)),
                 box(glm::vec3(0), glm::vec3(0))}) {
            const auto contact = Collider::intersect(1, box(), 2, other);
            CHECK(contact);
            if (contact) {
                CHECK(contains(box(), contact->position));
                CHECK(contains(other, contact->position));
                CHECK(contact->penetration_depth >= 0);
                CHECK(near(glm::length(contact->normal), 1));
            }
        }
        const auto overlap = Collider::intersect(1, box(), 2, box(glm::vec3(1.5, 0, 0)));
        CHECK(overlap && near(overlap->penetration_depth, 0.5f));
        CHECK(overlap && near(overlap->normal.x, 1.0f));
        const auto contained =
            Collider::intersect(1, box(), 2, box(glm::vec3(0), glm::vec3(0.25f)));
        CHECK(contained && near(contained->penetration_depth, 1.25f));
        CHECK(Collider::hit_model({{0, 0, -5}, {0, 0, 1}}, box()));
        CHECK(!Collider::hit_model({{0, 0, -5}, {0, 0, -1}}, box()));
        CHECK(!Collider::hit_model({{2, 0, -5}, {0, 0, 1}}, box()));
        CHECK(!Collider::hit_model({{0, 0, 0}, {0, 0, 0}}, box()));

        std::mt19937 random(8421);
        std::uniform_real_distribution<float> number(-1.0f, 1.0f);
        int hits = 0;
        int misses = 0;
        for (int iteration = 0; iteration < 500; ++iteration) {
            const auto make_random_box = [&] {
                const glm::vec3 center(number(random) * 3, number(random) * 3, number(random) * 3);
                const glm::vec3 extent(std::abs(number(random)) + 0.05f,
                    std::abs(number(random)) + 0.05f,
                    std::abs(number(random)) + 0.05f);
                const glm::vec3 axis =
                    glm::normalize(glm::vec3(number(random), number(random), number(random)));
                return box(
                    center, extent, glm::mat3(glm::rotate(glm::mat4(1), number(random) * 3, axis)));
            };
            const auto first = make_random_box();
            const auto second = make_random_box();
            const bool expected = reference_intersection(first, second);
            const auto actual = Collider::intersect(1, first, 2, second);
            CHECK(actual.has_value() == expected);
            expected ? ++hits : ++misses;
            if (actual) {
                CHECK(contains(first, actual->position));
                CHECK(contains(second, actual->position));
                const auto reverse = Collider::intersect(2, second, 1, first);
                CHECK(reverse);
                CHECK(reverse && near(actual->penetration_depth, reverse->penetration_depth));
                CHECK(reverse && glm::length(actual->normal + reverse->normal) < 1e-4f);
            }
        }
        CHECK(hits > 0 && misses > 0);
    }

    class Handler : public enishi::sub_system::ICollisionHandler {
      public:
        std::vector<std::pair<types::BoneIndex, types::Collision>> events;
        std::function<void()> callback;
        void on_collision(types::BoneIndex bone, const types::Collision& collision) override {
            this->events.emplace_back(bone, collision);
            if (this->callback) {
                this->callback();
            }
        }
    };

    void test_system() {
        Collider system;
        const auto points = vertices(box());
        const auto first_handler = std::make_shared<Handler>();
        const auto second_handler = std::make_shared<Handler>();
        CHECK(!system.set_bone(1, 0, points));
        CHECK(!system.add_handler(1, first_handler));
        CHECK(system.register_model(1));
        CHECK(!system.register_model(1));
        CHECK(system.add_handler(1, first_handler));
        CHECK(!system.add_handler(1, first_handler));
        CHECK(!system.add_handler(1, nullptr));
        CHECK(system.set_bone(1, 0, points));
        CHECK(system.set_bone(1, 1, points));
        CHECK(system.check_collisions() == 0);
        CHECK(first_handler->events.empty());
        CHECK(system.register_model(2));
        CHECK(system.set_bone(2, 42, points, glm::translate(glm::mat4(1), glm::vec3(1.5, 0, 0))));
        CHECK(system.add_handler(2, second_handler));
        CHECK(system.check_collisions() == 2);
        CHECK(first_handler->events.size() == 2 && second_handler->events.size() == 2);
        for (const auto& [bone, event] : first_handler->events) {
            CHECK((bone == 0 || bone == 1) && event.other_model == 2 && event.other_bone == 42);
            CHECK(near(event.contact.penetration_depth, 0.5f));
            CHECK(near(event.contact.normal.x, 1));
            CHECK(contains(system.get_obb_map(1)->at(bone), event.contact.position));
        }
        for (const auto& [bone, event] : second_handler->events) {
            CHECK(bone == 42 && event.other_model == 1);
            CHECK(near(event.contact.normal.x, -1));
        }
        CHECK(system.check_collisions() == 2);
        CHECK(first_handler->events.size() == 4 && second_handler->events.size() == 4);
        CHECK(!system.set_bone(2, 42, {}));
        CHECK(!system.set_bone(2, types::INVALID_BONE_INDEX, points));
        CHECK(system.get_obb_map(2)->size() == 1);
        CHECK(system.update_bone(2, 42, glm::translate(glm::mat4(1), glm::vec3(10, 0, 0))));
        CHECK(system.check_collisions() == 0);
        CHECK(system.update_bone(2, 42, glm::mat4(1)));
        CHECK(system.check_collisions() == 2);
        CHECK(!system.update_bone(2, 999, glm::mat4(1)));
        CHECK(system.remove_bone(1, 1));
        CHECK(!system.remove_bone(1, 1));
        CHECK(system.check_collisions() == 1);
        CHECK(system.remove_handler(1, first_handler));
        CHECK(!system.remove_handler(1, first_handler));
        const auto previous = first_handler->events.size();
        CHECK(system.check_collisions() == 1);
        CHECK(first_handler->events.size() == previous);
        CHECK(system.add_handler(1, first_handler));
        // Removal and rehashing during delivery must not invalidate the notification batch.
        first_handler->callback = [&] {
            system.remove_model(2);
            for (types::CollisionModelId id = 100; id < 200; ++id) {
                static_cast<void>(system.register_model(id));
            }
        };
        const auto second_previous = second_handler->events.size();
        CHECK(system.check_collisions() == 1);
        CHECK(second_handler->events.size() == second_previous + 1);
        CHECK(system.get_obb_map(2) == nullptr);
        CHECK(system.check_collisions() == 0);
    }
} // namespace

int main() {
    test_maker();
    test_intersection();
    test_system();
    if (failures != 0) {
        std::cerr << failures << " checks failed\n";
        return 1;
    }
    std::cout << "All collider checks passed (including 500 independent geometric comparisons).\n";
    return 0;
}
