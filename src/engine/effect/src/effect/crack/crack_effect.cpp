#include "crack_effect.h"
#include <algorithm>
#include <utility>
namespace enishi::effect {
    CrackEffect::CrackEffect(CrackSettings settings, std::unique_ptr<ICrackSurface> surface)
        : settings(settings)
        , surface(std::move(surface)) {
    }
    CrackEffect::~CrackEffect() noexcept {
        this->stop();
    }
    foundation::Result<void, EffectError> CrackEffect::start() {
        if (this->surface == nullptr || this->started) {
            return foundation::Error{
                EffectError::StartFailed, "Crack surface is missing or already active"};
        }
        auto mask = make_crack_mask(this->settings);
        if (mask.is_err()) {
            return std::move(mask).take_err();
        }
        auto opened = this->surface->open(this->settings, std::move(mask.unwrap_mut()));
        if (opened.is_err()) {
            this->surface->close();
            return std::move(opened).take_err();
        }
        this->elapsed_seconds = 0;
        this->started = true;
        auto drawn = this->surface->draw(this->settings.opacity);
        if (drawn.is_err()) {
            this->stop();
        }
        return drawn;
    }
    foundation::Result<EffectStatus, EffectError> CrackEffect::update(const types::DeltaTime& dt) {
        if (!this->started || dt.delta_time.count() < 0) {
            return foundation::Error{
                EffectError::UpdateFailed, "Crack effect is inactive or delta time is negative"};
        }
        this->elapsed_seconds += dt.to_double_second();
        const double remaining =
            static_cast<double>(this->settings.duration_seconds) - this->elapsed_seconds;
        if (!(remaining > 0)) {
            return EffectStatus::Finished;
        }
        float opacity = this->settings.opacity;
        if (this->settings.fade_seconds > 0) {
            // Linear fade: alpha = opacity * clamp(remaining / fade_duration, 0, 1).
            opacity *=
                static_cast<float>(std::clamp(remaining / this->settings.fade_seconds, 0.0, 1.0));
        }
        auto drawn = this->surface->draw(opacity);
        if (drawn.is_err()) {
            return std::move(drawn).take_err();
        }
        return EffectStatus::Running;
    }
    void CrackEffect::stop() noexcept {
        if (this->started) {
            this->surface->close();
            this->started = false;
        }
    }
} // namespace enishi::effect
