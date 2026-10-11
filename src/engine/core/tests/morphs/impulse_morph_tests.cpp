#include <core/system/animation/impulse_morph.h>
#include <cstdlib>
#include <iostream>
#include <limits>

using namespace enishi;
namespace {
    void check(bool condition, const char* message) {
        if (!condition) {
            std::cerr << message << '\n';
            std::exit(EXIT_FAILURE);
        }
    }
} // namespace

void impulse_morph_tests() {
    std::vector<types::MorphTarget> targets(3);
    targets[0].offsets = {types::ImpulseMorphOffset{0, false, {2, 0, 0}, {0, 4, 0}},
        types::ImpulseMorphOffset{0, true, {0, 6, 0}, {0, 0, 8}},
        types::ImpulseMorphOffset{0, false, {}, {}, true}};
    targets[1].offsets = {types::ImpulseMorphOffset{1, false, {1, 2, 3}, {}, false}};
    targets[2].offsets = {types::MorphWeightOffset{0, 0.5f}};
    const auto result = core::evaluate_impulse_morphs(2, targets, std::vector<float>{0, -1, 1});
    check(result.is_ok(), "evaluate grouped impulses");
    const auto& first = result.unwrap()[0];
    check(first.world_velocity == glm::vec3(1, 0, 0) && first.world_torque == glm::vec3(0, 2, 0) &&
              first.local_velocity == glm::vec3(0, 3, 0) &&
              first.local_torque == glm::vec3(0, 0, 4),
        "sum local and world impulses separately");
    check(first.reset_velocity && result.unwrap()[1].world_velocity == glm::vec3(-1, -2, -3),
        "reset is independent of summed impulses and negative weights reverse impulse");
    const auto zero = core::evaluate_impulse_morphs(2, targets, std::vector<float>{0, 0, 0});
    check(zero.is_ok() && !zero.unwrap()[0].reset_velocity &&
              zero.unwrap()[0].world_velocity == glm::vec3(0),
        "inactive impulses do not reset or accumulate");
    check(core::evaluate_impulse_morphs(1, targets, std::vector<float>{0, 1, 0}).is_err(),
        "reject missing body");
    targets[1].offsets = {types::ImpulseMorphOffset{
        0, true, glm::vec3(std::numeric_limits<float>::infinity()), {}, false}};
    check(core::evaluate_impulse_morphs(1, targets, std::vector<float>{0, 1, 0}).is_err(),
        "reject non-finite impulse");
}
