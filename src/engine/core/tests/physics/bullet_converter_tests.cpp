#include "bullet_converter_tests.h"
#include <array>
#include <cmath>
#include <cstddef>
#include <memory>
#include <new>
#include <physics/bullet3/converter/bullet_converter.h>
#include <test_support/test_runner.h>

void bullet_converter_tests() {
    using enishi::physics::bullet3::BulletConverter;
    constexpr std::size_t SIMD_ALIGNMENT = 16;
    constexpr float TOLERANCE = 0.0001f;
    const btTransform source(
        btQuaternion(btVector3(1, 2, 3).normalized(), btScalar(0.7)), btVector3(2, -3, 4));
    // Exercise every legal GLM placement inside a SIMD block, including embedded matrices.
    alignas(SIMD_ALIGNMENT) std::array<std::byte, sizeof(glm::mat4) + SIMD_ALIGNMENT> storage{};
    for (std::size_t offset = 0; offset < SIMD_ALIGNMENT; offset += alignof(glm::mat4)) {
        auto* matrix = ::new (static_cast<void*>(storage.data() + offset))
            glm::mat4(BulletConverter::transform_to_matrix(source));
        constexpr glm::length_t SPATIAL_DIMENSIONS = 3;
        for (glm::length_t column = 0; column < SPATIAL_DIMENSIONS; ++column) {
            for (glm::length_t row = 0; row < SPATIAL_DIMENSIONS; ++row) {
                enishi::tests::check(
                    std::abs((*matrix)[column][row] -
                             static_cast<float>(source.getBasis()[row][column])) < TOLERANCE,
                    "Bullet row-major basis maps to GLM column-major elements");
            }
            enishi::tests::check((*matrix)[column][SPATIAL_DIMENSIONS] == 0,
                "affine basis has zero homogeneous components");
            enishi::tests::check((*matrix)[SPATIAL_DIMENSIONS][column] ==
                                     static_cast<float>(source.getOrigin()[column]),
                "translation is preserved");
        }
        enishi::tests::check((*matrix)[SPATIAL_DIMENSIONS][SPATIAL_DIMENSIONS] == 1,
            "affine translation has unit homogeneous component");
        const auto round_trip = BulletConverter::matrix_to_transform(*matrix);
        enishi::tests::check((round_trip.getOrigin() - source.getOrigin()).length() < TOLERANCE,
            "Bullet translation survives the conversion round trip");
        for (int row = 0; row < SPATIAL_DIMENSIONS; ++row) {
            enishi::tests::check(
                (round_trip.getBasis()[row] - source.getBasis()[row]).length() < TOLERANCE,
                "Bullet rotation survives the conversion round trip");
        }
        std::destroy_at(matrix);
    }
}
