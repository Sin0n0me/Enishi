#include <cstdlib>
#include <glm/gtc/matrix_transform.hpp>
#include <ik_system/ik_solver.h>
#include <iostream>
#include <skinning_system/cache/bind_bone_cache.h>
#include <skinning_system/updater/ik_bones_updater.h>

using namespace enishi;

namespace {
    constexpr float TOLERANCE = 0.0001f;
    constexpr types::BoneIndex PIVOT = 0;
    constexpr types::BoneIndex JOINT = 1;
    constexpr types::BoneIndex TIP = 2;
    constexpr types::BoneIndex GOAL = 3;
    constexpr std::size_t BONE_COUNT = 4;

    void check(bool value, const char* message) {
        if (!value) {
            std::cerr << message << '\n';
            std::exit(EXIT_FAILURE);
        }
    }

    struct Fixture {
        std::vector<types::BoneNode> nodes{BONE_COUNT};
        std::vector<types::BindBone> binds{BONE_COUNT};
        std::vector<glm::quat> rotations =
            std::vector<glm::quat>(BONE_COUNT, glm::quat(1, 0, 0, 0));
        std::vector<glm::mat4> globals = std::vector<glm::mat4>(BONE_COUNT, glm::mat4(1));
        std::vector<glm::mat4> local = std::vector<glm::mat4>(BONE_COUNT, glm::mat4(1));
        std::unique_ptr<skinning_system::IKBoneCache> cache;
        std::unique_ptr<skinning_system::BindBonesCache> bind_cache;
        std::unique_ptr<skinning_system::IKBonesUpdater> updater;

        Fixture() {
            this->nodes[PIVOT].children = {JOINT};
            this->nodes[JOINT].parent = PIVOT;
            this->nodes[JOINT].children = {TIP};
            this->nodes[TIP].parent = JOINT;
            this->local[JOINT] = glm::translate(glm::mat4(1), glm::vec3(1, 0, 0));
            this->local[TIP] = this->local[JOINT];
            this->local[GOAL] = glm::translate(glm::mat4(1), glm::vec3(1, 1, 0));
            std::vector<std::unique_ptr<skinning_system::IKBoneView>> views;
            std::vector<std::unique_ptr<skinning_system::BindBoneView>> bind_views;
            for (std::size_t index = 0; index < BONE_COUNT; ++index) {
                views.push_back(std::make_unique<skinning_system::IKBoneView>(
                    this->rotations[index], this->globals[index]));
                auto& bind = this->binds[index];
                bind_views.push_back(std::make_unique<skinning_system::BindBoneView>(
                    bind.local, bind.global, bind.global_inverse));
            }
            this->cache =
                std::make_unique<skinning_system::IKBoneCache>(this->nodes, std::move(views));
            this->bind_cache =
                std::make_unique<skinning_system::BindBonesCache>(std::move(bind_views));
            this->updater = std::make_unique<skinning_system::IKBonesUpdater>(
                *this->cache, *this->bind_cache, this->local);
            this->updater->update_global_form_roots();
        }

        void solve(types::IkLimit limit = types::IKLimitAngle{glm::quarter_pi<float>()},
            std::vector<types::IKLinkLimit> bounds = {}) {
            types::CCDIK chain{};
            chain.ik_bone = GOAL;
            chain.target = TIP;
            chain.chain = {JOINT};
            chain.iterations = 8;
            chain.limit = limit;
            chain.link_limits = std::move(bounds);
            ik::IKSolver::apply_ik(
                types::IK{chain}, this->cache.get(), this->updater.get(), GOAL, this->local);
        }
    };

    void chain_test() {
        Fixture fixture;
        const auto goal = fixture.globals[GOAL];
        fixture.solve();
        check(glm::length(glm::vec3(fixture.globals[TIP][3] - goal[3])) < TOLERANCE,
            "CCD must accumulate link rotations and reach the goal");
        check(fixture.rotations[GOAL] == glm::quat(1, 0, 0, 0) && fixture.globals[GOAL] == goal,
            "controller bone must not receive chain rotations");
        check(glm::length(fixture.rotations[JOINT] * glm::vec3(1, 0, 0) - glm::vec3(0, 1, 0)) <
                  TOLERANCE,
            "the actual chain joint must receive the rotation");
    }

    void animated_parent_test() {
        Fixture fixture;
        fixture.local[PIVOT] = glm::translate(glm::mat4(1), glm::vec3(4, 5, 0)) *
                               glm::rotate(glm::mat4(1), glm::half_pi<float>(), glm::vec3(0, 0, 1));
        fixture.local[GOAL] = glm::translate(glm::mat4(1), glm::vec3(5, 6, 0));
        fixture.updater->update_global_form_roots();
        fixture.solve();
        check(glm::length(glm::vec3(fixture.globals[TIP][3]) - glm::vec3(5, 6, 0)) < TOLERANCE,
            "IK must preserve animated parent translation and rotation");
    }

    void boundary_tests() {
        Fixture fixture;
        fixture.local[GOAL] = glm::translate(glm::mat4(1), glm::vec3(0, 0, 0));
        fixture.updater->update_global_form_roots();
        fixture.solve(types::IKLimitAngle{glm::pi<float>()});
        check(glm::length(glm::vec3(fixture.globals[TIP][3])) < TOLERANCE,
            "opposite directions must produce a finite half turn");
        Fixture zero_length;
        zero_length.local[TIP] = glm::mat4(1);
        zero_length.updater->update_global_form_roots();
        zero_length.solve();
        check(
            std::isfinite(zero_length.globals[TIP][3].x), "zero-length direction must stay finite");
        Fixture axis_limited;
        axis_limited.local[GOAL] = glm::translate(glm::mat4(1), glm::vec3(1, -1, 0));
        axis_limited.updater->update_global_form_roots();
        axis_limited.solve(types::IKLimitAxis{glm::vec3(0, 0, 1), glm::quarter_pi<float>()});
        check(
            glm::length(glm::vec3(axis_limited.globals[TIP][3]) - glm::vec3(1, -1, 0)) < TOLERANCE,
            "axis-constrained rotation must retain its sign");
    }

    void link_limit_tests() {
        const auto quarter = glm::quarter_pi<float>();
        Fixture limited;
        limited.solve(types::IKLimitAngle{quarter}, {{true, {0, 0, -quarter}, {0, 0, quarter}}});
        check(std::abs(glm::eulerAngles(limited.rotations[JOINT]).z - quarter) < TOLERANCE,
            "unreachable goal must stop at the per-link upper bound");

        Fixture animated;
        animated.local[JOINT] *= glm::rotate(glm::mat4(1), quarter, glm::vec3(0, 0, 1));
        animated.updater->update_global_form_roots();
        animated.solve(types::IKLimitAngle{quarter}, {{true, {0, 0, -quarter}, {0, 0, quarter}}});
        check(std::abs(animated.rotations[JOINT].w - 1.0f) < TOLERANCE,
            "link limits apply to animation plus IK, not the correction alone");

        Fixture negative;
        negative.local[GOAL] = glm::translate(glm::mat4(1), glm::vec3(1, -1, 0));
        negative.updater->update_global_form_roots();
        negative.solve(types::IKLimitAngle{quarter}, {{true, {0, 0, -quarter}, {0, 0, 0}}});
        check(std::abs(glm::eulerAngles(negative.rotations[JOINT]).z + quarter) < TOLERANCE,
            "negative hinge range must keep its sign");

        Fixture locked;
        locked.solve(types::IKLimitAngle{quarter}, {{true, {0, 0, 0}, {0, 0, 0}}});
        check(locked.rotations[JOINT] == glm::quat(1, 0, 0, 0), "locked link must stay fixed");
        Fixture disabled;
        disabled.solve(types::IKLimitAngle{quarter}, {{false, {0, 0, 0}, {0, 0, 0}}});
        check(glm::length(glm::vec3(disabled.globals[TIP][3] - disabled.globals[GOAL][3])) <
                  TOLERANCE,
            "disabled limits must preserve unrestricted CCD");
    }
} // namespace

int main() {
    chain_test();
    animated_parent_test();
    boundary_tests();
    link_limit_tests();
    std::cout << "IK runtime tests passed\n";
}
