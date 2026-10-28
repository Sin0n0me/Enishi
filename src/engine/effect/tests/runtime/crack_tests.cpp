#include "crack_tests.h"
#include "../../../core/tests/test_runner.h"
#include <array>
#include <cmath>
#include <effect/crack/crack_effect.h>
#include <effect/effect_manager.h>
#include <limits>
#include <numbers>
namespace enishi::tests {
    namespace {
        struct SurfaceState {
            unsigned opens = 0;
            unsigned closes = 0;
            unsigned hides = 0;
            unsigned draws = 0;
            float opacity = 0;
            std::optional<effect::CrackImpact> impact;
            bool fail_open = false;
            bool fail_draw = false;
        };
        class Surface final : public effect::ICrackSurface {
          private:
            std::shared_ptr<SurfaceState> state;

          public:
            explicit Surface(std::shared_ptr<SurfaceState> state);
            foundation::Result<void, effect::EffectError> open(
                const effect::CrackSettings&, types::TextureData mask) override;
            foundation::Result<void, effect::EffectError> draw(
                float opacity, std::optional<effect::CrackImpact> impact) override;
            void hide() noexcept override;
            void close() noexcept override;
        };
        Surface::Surface(std::shared_ptr<SurfaceState> state)
            : state(std::move(state)) {
        }
        foundation::Result<void, effect::EffectError> Surface::open(
            const effect::CrackSettings&, types::TextureData mask) {
            ++this->state->opens;
            check(!mask.mips.empty(), "surface receives generated mask");
            if (this->state->fail_open) {
                return foundation::Error(effect::EffectError::StartFailed, "open failure");
            }
            return {};
        }
        foundation::Result<void, effect::EffectError> Surface::draw(
            float opacity, std::optional<effect::CrackImpact> impact) {
            ++this->state->draws;
            this->state->opacity = opacity;
            this->state->impact = impact;
            if (this->state->fail_draw) {
                return foundation::Error(effect::EffectError::UpdateFailed, "draw failure");
            }
            return {};
        }
        void Surface::hide() noexcept {
            ++this->state->hides;
        }
        void Surface::close() noexcept {
            ++this->state->closes;
        }
        void mask_properties() {
            effect::CrackSettings settings;
            settings.mask_size = 64;
            settings.cells = 3;
            settings.pattern = effect::CrackPattern::Voronoi;
            auto first = effect::make_crack_mask(settings);
            auto second = effect::make_crack_mask(settings);
            check(first.is_ok() && second.is_ok(), "valid masks generate");
            const auto& texture = first.unwrap();
            const auto& mip = texture.mips.front();
            check(texture.width == 64 && texture.height == 64 && texture.mips.size() == 1,
                "mask dimensions and mip count");
            check(mip.row_pitch == 256 && mip.slice_pitch == 16384 && mip.pixels.size() == 16384,
                "RGBA mask pitches and allocation");
            check(mip.pixels == second.unwrap().mips.front().pixels, "same seed is deterministic");
            ++settings.seed;
            auto different = effect::make_crack_mask(settings);
            check(different.is_ok() && mip.pixels != different.unwrap().mips.front().pixels,
                "seed changes pattern");
            bool intact = false;
            bool groove = false;
            bool feather = false;
            for (std::size_t offset = 0; offset < mip.pixels.size(); offset += 4) {
                const auto depth = mip.pixels[offset];
                const auto coverage = mip.pixels[offset + 1];
                check(mip.pixels[offset + 2] == 0 && mip.pixels[offset + 3] == 0,
                    "unused channels are initialized");
                check(depth == 0 || coverage == 255, "depth is confined to fully covered grooves");
                intact = intact || coverage == 0;
                groove = groove || depth > 0;
                feather = feather || (coverage > 0 && coverage < 255);
            }
            check(intact && groove && feather,
                "mask has intact regions, recessed grooves and soft edges");
        }
        unsigned radial_crossings(const types::MipData& mip, float begin, float end) {
            constexpr unsigned ANGLES = 96;
            constexpr unsigned SAMPLES = 128;
            constexpr unsigned CHANNELS = 4;
            constexpr unsigned COVERAGE_CHANNEL = 1;
            constexpr unsigned COVERAGE_THRESHOLD = 128;
            unsigned crossings = 0;
            for (unsigned direction = 0; direction < ANGLES; ++direction) {
                const float angle =
                    2.0f * std::numbers::pi_v<float> * static_cast<float>(direction) / ANGLES;
                bool previous = false;
                for (unsigned step = 0; step < SAMPLES; ++step) {
                    const float radius = begin + (end - begin) * static_cast<float>(step) / SAMPLES;
                    // Project equally spaced radial samples into the centered [-1, 1] mask.
                    const auto x =
                        static_cast<unsigned>((0.5f + 0.5f * radius * std::cos(angle)) * mip.width);
                    const auto y = static_cast<unsigned>(
                        (0.5f + 0.5f * radius * std::sin(angle)) * mip.height);
                    const bool covered =
                        mip.pixels[y * mip.row_pitch + x * CHANNELS + COVERAGE_CHANNEL] >
                        COVERAGE_THRESHOLD;
                    if (step != 0 && covered && !previous) {
                        ++crossings;
                    }
                    previous = covered;
                }
            }
            return crossings;
        }
        void radial_pattern() {
            effect::CrackSettings settings;
            auto first = effect::make_crack_mask(settings);
            auto second = effect::make_crack_mask(settings);
            check(first.is_ok() && second.is_ok(), "radial masks generate");
            const auto& mip = first.unwrap().mips.front();
            check(mip.pixels == second.unwrap().mips.front().pixels,
                "radial pattern is reproducible");
            const auto near = radial_crossings(mip, 0.1f, 0.35f);
            const auto far = radial_crossings(mip, 0.65f, 0.9f);
            check(near > far && far > 0, "equal radial spans have more fractures near the impact");
            ++settings.seed;
            auto changed = effect::make_crack_mask(settings);
            check(changed.is_ok() && changed.unwrap().mips.front().pixels != mip.pixels,
                "seed varies radial pattern");
            settings.radial_spokes = 0;
            check(effect::make_crack_mask(settings).is_err(), "zero spokes rejected");
            settings.radial_spokes = 24;
            settings.ring_growth = 1;
            check(effect::make_crack_mask(settings).is_err(), "degenerate log growth rejected");
            settings.ring_growth = 1.6f;
            settings.impact_radius = 0;
            check(effect::make_crack_mask(settings).is_err(), "zero impact radius rejected");
        }
        void invalid_settings() {
            const std::array members{&effect::CrackSettings::width,
                &effect::CrackSettings::feather,
                &effect::CrackSettings::depth,
                &effect::CrackSettings::impact_radius,
                &effect::CrackSettings::ring_growth,
                &effect::CrackSettings::view_x,
                &effect::CrackSettings::view_y,
                &effect::CrackSettings::duration_seconds,
                &effect::CrackSettings::fade_seconds,
                &effect::CrackSettings::opacity};
            for (auto member : members) {
                for (const auto value : {std::numeric_limits<float>::infinity(),
                         -std::numeric_limits<float>::infinity(),
                         std::numeric_limits<float>::quiet_NaN()}) {
                    effect::CrackSettings settings;
                    settings.*member = value;
                    check(effect::make_crack_mask(settings).is_err(),
                        "nonfinite settings rejected before allocation");
                }
            }
            effect::CrackSettings settings;
            settings.mask_size = std::numeric_limits<std::uint32_t>::max();
            check(effect::make_crack_mask(settings).is_err(), "oversized allocation rejected");
            settings.mask_size = 32;
            settings.cells = 0;
            check(effect::make_crack_mask(settings).is_err(), "zero cells rejected");
            settings.cells = 1;
            settings.depth = 0;
            settings.fade_seconds = 0;
            check(effect::make_crack_mask(settings).is_ok(),
                "minimum size, zero depth and no fade supported");
            settings.fade_seconds = settings.duration_seconds + 1;
            check(effect::make_crack_mask(settings).is_err(), "fade beyond lifetime rejected");
        }
        void crack_lifecycle() {
            auto state = std::make_shared<SurfaceState>();
            effect::CrackSettings settings;
            settings.mask_size = 32;
            settings.duration_seconds = 2;
            settings.fade_seconds = 1;
            effect::CrackEffect effect{settings, std::make_unique<Surface>(state)};
            check(effect.start().is_ok(), "start opens and draws");
            check(effect.start().is_err() && state->opens == 1, "double start rejected");
            check(effect.update(types::DeltaTime{std::chrono::milliseconds{-1}}).is_err(),
                "negative elapsed time rejected");
            auto running = effect.update(types::DeltaTime{std::chrono::milliseconds{1500}});
            check(running.is_ok() && running.unwrap() == effect::EffectStatus::Running,
                "effect runs during fade");
            check(std::abs(state->opacity - settings.opacity * 0.5f) < 0.00001f,
                "linear fade uses remaining lifetime");
            const auto draws = state->draws;
            auto ended = effect.update(types::DeltaTime{std::chrono::milliseconds{500}});
            check(ended.is_ok() && ended.unwrap() == effect::EffectStatus::Finished &&
                      state->draws == draws,
                "exact duration finishes without drawing another frame");
            effect.stop();
            effect.stop();
            check(state->closes == 1, "stop closes once");
            check(effect.start().is_ok(), "stopped effect can restart");
            check(state->opacity == settings.opacity, "restart resets fade");
        }
        void prepared_playback() {
            auto state = std::make_shared<SurfaceState>();
            effect::CrackSettings settings;
            settings.mask_size = 32;
            {
                auto crack = std::make_shared<effect::CrackEffect>(
                    settings, std::make_unique<Surface>(state));
                effect::EffectManager manager;
                check(crack->prepare().is_ok() && crack->prepare().is_ok(),
                    "preparation is idempotent");
                check(state->opens == 1 && state->draws == 0, "prepare does not show the effect");
                for (int repeat = 0; repeat < 3; ++repeat) {
                    crack->set_impact(effect::CrackImpact{repeat * 100, -200});
                    auto id = manager.play_shared(crack);
                    check(state->impact.has_value() && state->impact->x == repeat * 100 &&
                              state->impact->y == -200,
                        "replay forwards a new desktop origin without reopening resources");
                    check(id.is_ok(), "prepared playback starts");
                    check(manager.play_shared(crack).is_err(), "same instance cannot play twice");
                    check(crack->prepare().is_err(), "active preparation rejected");
                    check(state->hides == static_cast<unsigned>(repeat),
                        "rejection does not stop active playback");
                    check(manager.stop(id.unwrap()).is_ok(), "prepared playback stops");
                    check(state->opens == 1 && state->closes == 0, "restart reuses resources");
                }
            }
            check(state->closes == 1 && state->hides == 3,
                "retained resources released on destruction");
        }
        void crack_failures() {
            for (const bool fail_open : {false, true}) {
                auto state = std::make_shared<SurfaceState>();
                state->fail_open = fail_open;
                state->fail_draw = !fail_open;
                effect::CrackSettings settings;
                settings.mask_size = 32;
                {
                    effect::CrackEffect effect{settings, std::make_unique<Surface>(state)};
                    check(effect.start().is_err(), "start propagates surface failure");
                }
                check(state->closes == 1, "failed start and destructor close surface exactly once");
            }
            effect::CrackEffect missing{{}, nullptr};
            check(missing.start().is_err(), "missing surface rejected");
        }
    } // namespace
    void run_crack_tests() {
        run_test("Voronoi crack mask invariants", mask_properties);
        run_test("Radial fracture density", radial_pattern);
        run_test("Crack settings validation", invalid_settings);
        run_test("Crack fade and lifetime", crack_lifecycle);
        run_test("Crack surface failure cleanup", crack_failures);
        run_test("Prepared crack playback", prepared_playback);
    }
} // namespace enishi::tests
