#include "effect_runtime_tests.h"
#include "../../../core/tests/test_runner.h"
#include <chrono>
#include <effect/effect_manager.h>
#include <functional>
#include <limits>
#include <type_traits>

namespace enishi::tests {
    struct EffectManagerTestAccess {
        static void exhaust_on_next_play(effect::EffectManager& manager);
    };
    void EffectManagerTestAccess::exhaust_on_next_play(effect::EffectManager& manager) {
        manager.next_id = std::numeric_limits<effect::EffectId::Value>::max();
    }
    namespace {
        using effect::EffectError;
        using effect::EffectId;
        using effect::EffectManager;
        using effect::EffectStatus;
        const types::DeltaTime FRAME{std::chrono::milliseconds(16)};
        struct State {
            unsigned starts = 0;
            unsigned updates = 0;
            unsigned stops = 0;
            unsigned destroyed = 0;
            bool fail_start = false;
            bool fail_update = false;
            bool finished = false;
            std::chrono::microseconds elapsed{};
            std::function<void()> on_start;
            std::function<void()> on_update;
            std::function<void()> on_stop;
        };
        class FakeEffect final : public effect::IEffect {
          private:
            std::shared_ptr<State> state;

          public:
            explicit FakeEffect(std::shared_ptr<State> state);
            ~FakeEffect() noexcept override;
            foundation::Result<void, EffectError> start() override;
            foundation::Result<EffectStatus, EffectError> update(
                const types::DeltaTime& dt) override;
            void stop() noexcept override;
        };
        FakeEffect::FakeEffect(std::shared_ptr<State> state)
            : state(std::move(state)) {
        }
        FakeEffect::~FakeEffect() noexcept {
            ++this->state->destroyed;
        }
        foundation::Result<void, EffectError> FakeEffect::start() {
            ++this->state->starts;
            if (this->state->on_start != nullptr) {
                this->state->on_start();
            }
            if (this->state->fail_start) {
                return foundation::Error{EffectError::StartFailed, "start diagnostic"};
            }
            return {};
        }
        foundation::Result<EffectStatus, EffectError> FakeEffect::update(
            const types::DeltaTime& dt) {
            ++this->state->updates;
            this->state->elapsed += dt.delta_time;
            if (this->state->on_update != nullptr) {
                this->state->on_update();
            }
            if (this->state->fail_update) {
                return foundation::Error{EffectError::UpdateFailed, "update diagnostic"};
            }
            return this->state->finished ? EffectStatus::Finished : EffectStatus::Running;
        }
        void FakeEffect::stop() noexcept {
            ++this->state->stops;
            if (this->state->on_stop != nullptr) {
                this->state->on_stop();
            }
        }
        EffectId play(effect::IEffectPlayer& player, const std::shared_ptr<State>& state) {
            auto result = player.play(std::make_unique<FakeEffect>(state));
            check(result.is_ok(), "play must succeed");
            check(result.unwrap().is_valid(), "play must return a valid ID");
            return result.unwrap();
        }
        void update(EffectManager& manager) {
            auto result = manager.update(FRAME);
            check(result.is_ok(), "update must succeed");
            check(result.unwrap().empty(), "unexpected effect failures");
        }
        void lifecycle() {
            auto first = std::make_shared<State>();
            auto second = std::make_shared<State>();
            auto third = std::make_shared<State>();
            {
                EffectManager manager;
                effect::IEffectPlayer& player = manager;
                const auto a = play(player, first);
                const auto b = play(player, second);
                check(a != b && player.is_playing(a) && player.is_playing(b), "independent IDs");
                update(manager);
                check(
                    first->starts == 1 && first->updates == 1 && first->elapsed == FRAME.delta_time,
                    "start once and forward frame time");
                first->finished = true;
                update(manager);
                check(
                    !player.is_playing(a) && player.is_playing(b), "natural completion isolation");
                check(first->stops == 1 && first->destroyed == 1, "natural completion cleanup");
                auto stale = player.stop(a);
                check(stale.is_ok() && !stale.unwrap(), "finished ID is inactive");
                auto stopped = player.stop(b);
                check(stopped.is_ok() && stopped.unwrap(), "explicit stop");
                check(second->stops == 1 && second->destroyed == 1, "explicit stop cleanup");
                const auto c = play(player, third);
                check(c != a && c != b, "never reuse IDs");
            }
            check(third->stops == 1 && third->destroyed == 1, "manager destruction cleanup");
            check(first->stops == 1 && second->stops == 1, "no double cleanup on destruction");
        }
        void failure_isolation() {
            EffectManager manager;
            auto failed_start = std::make_shared<State>();
            failed_start->fail_start = true;
            auto rejected = manager.play(std::make_unique<FakeEffect>(failed_start));
            check(
                rejected.is_err() && rejected.unwrap_err().get_error() == EffectError::StartFailed,
                "preserve start failure category");
            check(rejected.unwrap_err().get_message() == "start diagnostic",
                "preserve start diagnostic");
            check(failed_start->stops == 1 && failed_start->destroyed == 1, "failed start cleanup");
            auto failing = std::make_shared<State>();
            auto survivor = std::make_shared<State>();
            auto also_failing = std::make_shared<State>();
            failing->fail_update = true;
            also_failing->fail_update = true;
            const auto a = play(manager, failing);
            const auto b = play(manager, survivor);
            const auto c = play(manager, also_failing);
            auto result = manager.update(FRAME);
            check(result.is_ok() && result.unwrap().size() == 2, "collect all failures");
            check(result.unwrap()[0].id == a && result.unwrap()[1].id == c,
                "associate failures with IDs");
            for (const auto& failure : result.unwrap()) {
                check(failure.error.get_error() == EffectError::UpdateFailed &&
                          failure.error.get_message() == "update diagnostic",
                    "preserve update diagnostics");
            }
            check(!manager.is_playing(a) && manager.is_playing(b) && !manager.is_playing(c),
                "failed effects removed without stopping others");
            check(failing->stops == 1 && also_failing->stops == 1 && survivor->updates == 1,
                "all effects processed once");
            update(manager);
            check(survivor->updates == 2, "survivor keeps running");
        }
        void invalid_arguments() {
            EffectManager manager;
            auto empty = manager.play(nullptr);
            check(empty.is_err() && empty.unwrap_err().get_error() == EffectError::InvalidArgument,
                "reject null effect");
            auto stopped = manager.stop(EffectId{});
            check(stopped.is_ok() && !stopped.unwrap() && !manager.is_playing(EffectId{}),
                "invalid ID");
            const auto state = std::make_shared<State>();
            const auto id = play(manager, state);
            const types::DeltaTime negative{std::chrono::microseconds(-1)};
            auto bad_delta = manager.update(negative);
            check(bad_delta.is_err() &&
                      bad_delta.unwrap_err().get_error() == EffectError::InvalidArgument,
                "negative delta rejected");
            check(state->updates == 0 && manager.is_playing(id),
                "bad delta must not mutate playback");
            auto zero_delta = manager.update(types::DeltaTime{std::chrono::microseconds::zero()});
            check(zero_delta.is_ok() && state->updates == 1 && state->elapsed.count() == 0,
                "zero delta accepted");
            auto cleared = manager.stop_all();
            check(cleared.is_ok() && state->stops == 1 && !manager.is_playing(id), "stop all");
            auto again = manager.stop_all();
            check(again.is_ok() && state->stops == 1, "stop all is idempotent");
            update(manager);
        }
        void id_exhaustion() {
            EffectManager manager;
            const auto first = play(manager, std::make_shared<State>());
            EffectManagerTestAccess::exhaust_on_next_play(manager);
            const auto last = play(manager, std::make_shared<State>());
            check(last.value == std::numeric_limits<EffectId::Value>::max(), "last ID is usable");
            const auto rejected_state = std::make_shared<State>();
            auto rejected = manager.play(std::make_unique<FakeEffect>(rejected_state));
            check(
                rejected.is_err() && rejected.unwrap_err().get_error() == EffectError::IdExhausted,
                "ID exhaustion must not wrap");
            check(rejected_state->starts == 0 && rejected_state->stops == 0 &&
                      rejected_state->destroyed == 1,
                "rejected instance is destroyed without starting");
            check(manager.is_playing(first) && manager.is_playing(last),
                "exhaustion preserves existing effects");
        }
        void reentrancy() {
            EffectManager manager;
            const auto state = std::make_shared<State>();
            EffectId id;
            unsigned callbacks = 0;
            const auto verify_busy = [&] {
                ++callbacks;
                auto play_result =
                    manager.play(std::make_unique<FakeEffect>(std::make_shared<State>()));
                auto stop_result = manager.stop(id);
                auto all_result = manager.stop_all();
                auto update_result = manager.update(FRAME);
                check(play_result.is_err() &&
                          play_result.unwrap_err().get_error() == EffectError::Busy,
                    "reject reentrant play");
                check(stop_result.is_err() &&
                          stop_result.unwrap_err().get_error() == EffectError::Busy,
                    "reject reentrant stop");
                check(
                    all_result.is_err() && all_result.unwrap_err().get_error() == EffectError::Busy,
                    "reject reentrant stop_all");
                check(update_result.is_err() &&
                          update_result.unwrap_err().get_error() == EffectError::Busy,
                    "reject reentrant update");
            };
            state->on_start = verify_busy;
            state->on_update = verify_busy;
            state->on_stop = [&] {
                verify_busy();
                check(!manager.is_playing(id), "removed before cleanup callback");
            };
            id = play(manager, state);
            update(manager);
            auto stopped = manager.stop(id);
            check(stopped.is_ok() && callbacks == 3, "all lifecycle callbacks reject mutation");
            const auto next = play(manager, std::make_shared<State>());
            check(manager.is_playing(next), "guard resets after callbacks");
        }
        void bulk_cleanup() {
            constexpr unsigned EFFECT_COUNT = 32;
            for (const bool destroy_manager : {false, true}) {
                auto manager = std::make_unique<EffectManager>();
                std::vector<std::shared_ptr<State>> states;
                for (unsigned index = 0; index < EFFECT_COUNT; ++index) {
                    auto state = std::make_shared<State>();
                    const auto id = play(*manager, state);
                    state->on_stop = [&, id] {
                        check(!manager->is_playing(id), "cleanup ID is already inactive");
                        auto reentrant = manager->stop_all();
                        check(reentrant.is_err() &&
                                  reentrant.unwrap_err().get_error() == EffectError::Busy,
                            "bulk cleanup rejects reentry");
                    };
                    states.push_back(std::move(state));
                }
                if (destroy_manager) {
                    // Keep the pointer observable during the destructor's callbacks.
                    auto* instance = manager.get();
                    for (auto& state : states) {
                        state->on_stop = [instance] {
                            auto nested = instance->stop_all();
                            check(nested.is_err() &&
                                      nested.unwrap_err().get_error() == EffectError::Busy,
                                "destructor rejects reentry");
                        };
                    }
                    manager.reset();
                } else {
                    auto stopped = manager->stop_all();
                    check(stopped.is_ok(), "bulk stop succeeds");
                }
                for (const auto& state : states) {
                    check(state->stops == 1 && state->destroyed == 1,
                        "each instance cleaned exactly once");
                }
            }
        }
        void simultaneous_completion() {
            constexpr unsigned EFFECT_COUNT = 1024;
            EffectManager manager;
            std::vector<std::shared_ptr<State>> states;
            for (unsigned index = 0; index < EFFECT_COUNT; ++index) {
                auto state = std::make_shared<State>();
                state->finished = true;
                static_cast<void>(play(manager, state));
                states.push_back(std::move(state));
            }
            update(manager);
            for (const auto& state : states) {
                check(state->updates == 1 && state->stops == 1 && state->destroyed == 1,
                    "removing entries while updating must not skip entries");
            }
            update(manager);
        }
    } // namespace
} // namespace enishi::tests
int main() {
    enishi::tests::run_test("Lifecycle through playback interface", enishi::tests::lifecycle);
    enishi::tests::run_test("Failure diagnostics and isolation", enishi::tests::failure_isolation);
    enishi::tests::run_test("Invalid arguments and stop_all", enishi::tests::invalid_arguments);
    enishi::tests::run_test("ID exhaustion", enishi::tests::id_exhaustion);
    enishi::tests::run_test("Lifecycle reentrancy", enishi::tests::reentrancy);
    enishi::tests::run_test("Bulk stop and destruction reentry", enishi::tests::bulk_cleanup);
    enishi::tests::run_test(
        "Simultaneous natural completion", enishi::tests::simultaneous_completion);
}
