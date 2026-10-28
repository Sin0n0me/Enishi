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
        this->release();
    }
    foundation::Result<void, EffectError> CrackEffect::prepare() {
        if (this->started) {
            return foundation::Error{EffectError::Busy, "Cannot prepare an active crack effect"};
        }
        auto result = this->open();
        if (result.is_ok()) {
            this->reusable = true;
        }
        return result;
    }
    foundation::Result<void, EffectError> CrackEffect::start() {
        if (this->surface == nullptr || this->started) {
            return foundation::Error{
                EffectError::StartFailed, "Crack surface is missing or already active"};
        }
        auto opened = this->open();
        if (opened.is_err()) {
            return opened;
        }
        this->elapsed_seconds = 0;
        this->started = true;
        auto drawn = this->surface->draw(this->settings.opacity, this->impact, this->target);
        if (drawn.is_err()) {
            this->release();
        }
        return drawn;
    }
    foundation::Result<void, EffectError> CrackEffect::open() {
        if (this->resources_open) {
            return {};
        }
        if (this->surface == nullptr) {
            return foundation::Error{EffectError::StartFailed, "Crack surface is missing"};
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
        this->resources_open = true;
        return {};
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
        auto drawn = this->surface->draw(opacity, this->impact, this->target);
        if (drawn.is_err()) {
            this->release();
            return std::move(drawn).take_err();
        }
        return EffectStatus::Running;
    }
    void CrackEffect::set_impact(std::optional<CrackImpact> origin) noexcept {
        this->impact = origin;
    }
    void CrackEffect::set_target(const EffectTarget& destination) noexcept {
        this->target = destination;
    }
    void CrackEffect::stop() noexcept {
        if (this->started) {
            if (!this->reusable || !this->surface->hide()) {
                this->release();
            }
            this->started = false;
        }
    }
    void CrackEffect::release() noexcept {
        if (this->resources_open) {
            this->surface->close();
            this->resources_open = false;
        }
        this->started = false;
    }
} // namespace enishi::effect
