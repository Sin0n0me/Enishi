#include "crack_surface.h"
#include "effect_surface.h"
#include <algorithm>
#include <cmath>
#include <render_pass/crack_pass.h>
namespace enishi::platform_impl {
    namespace {
        foundation::Result<void, effect::EffectError> validate_surface_input(
            const effect::CrackSettings& settings, const types::TextureData& mask) {
            auto valid = effect::validate_crack_settings(settings);
            if (valid.is_err()) {
                return valid;
            }
            constexpr std::uint32_t RGBA_CHANNELS = 4;
            // A tightly packed mask requires row_bytes = width * channels and total = row_bytes *
            // height.
            const auto row_bytes = settings.mask_size * RGBA_CHANNELS;
            const auto total_bytes = row_bytes * settings.mask_size;
            if (mask.format != types::TextureFormat::RGBA8_UNORM || mask.is_cubemap ||
                mask.width != settings.mask_size || mask.height != settings.mask_size ||
                mask.depth != 1 || mask.array_size != 1 || mask.mips.size() != 1) {
                return foundation::Error(
                    effect::EffectError::InvalidArgument, "Invalid crack surface mask layout");
            }
            const auto& mip = mask.mips.front();
            if (mip.width != mask.width || mip.height != mask.height ||
                mip.row_pitch != row_bytes || mip.slice_pitch != total_bytes ||
                mip.pixels.size() != total_bytes) {
                return foundation::Error(
                    effect::EffectError::InvalidArgument, "Invalid crack surface mask storage");
            }
            return {};
        }
        struct CrackFrame {
            float opacity = 0;
            std::optional<effect::CrackImpact> impact;
        };
        class CrackContent final : public IEffectSurfaceContent {
          private:
            effect::CrackSettings settings;
            std::shared_ptr<platform::IShaderDataProvider> shaders;
            std::shared_ptr<types::TextureData> mask;
            std::shared_ptr<CrackFrame> state;
            std::unique_ptr<render_pass::CrackPass> pass;

          public:
            CrackContent(effect::CrackSettings settings,
                std::shared_ptr<platform::IShaderDataProvider> shaders,
                std::shared_ptr<types::TextureData> mask,
                std::shared_ptr<CrackFrame> state);
            platform::RenderResult<void> initialize(const EffectSurfaceContext& context) override;
            platform::RenderResult<void> draw(const EffectSurfaceFrame& frame) override;
        };
        CrackContent::CrackContent(effect::CrackSettings settings,
            std::shared_ptr<platform::IShaderDataProvider> shaders,
            std::shared_ptr<types::TextureData> mask,
            std::shared_ptr<CrackFrame> state)
            : settings(settings)
            , shaders(std::move(shaders))
            , mask(std::move(mask))
            , state(std::move(state)) {
        }
        platform::RenderResult<void> CrackContent::initialize(const EffectSurfaceContext& context) {
            this->pass = std::make_unique<render_pass::CrackPass>(context.renderer,
                context.encoder,
                render_pass::CrackPass::Settings{this->settings.view_x,
                    this->settings.view_y,
                    this->settings.depth,
                    this->settings.mask_size,
                    this->settings.opacity});
            auto built = this->pass->initialize(
                context.window, *this->shaders, context.backend, std::move(this->mask));
            if (built.is_err()) {
                return built;
            }
            // Submit while hidden so shader/upload work happens before interactive playback.
            return this->pass->draw(0.0f);
        }
        platform::RenderResult<void> CrackContent::draw(const EffectSurfaceFrame& frame) {
            glm::vec4 transform{1, 1, 0, 0};
            if (this->settings.pattern == effect::CrackPattern::Radial) {
                constexpr float MASK_CENTER = 0.5f;
                const auto& impact = this->state->impact;
                const float span = impact.has_value() && frame.all_displays
                                       ? frame.desktop_span
                                       : static_cast<float>(std::max(frame.width, frame.height));
                // mask_uv = 0.5 + (desktop_pixel - impact_pixel) / (2 * desktop_span).
                const float scale = MASK_CENTER / span;
                const float origin_x = impact.has_value() ? static_cast<float>(impact->x)
                                                          : frame.x + frame.width * MASK_CENTER;
                const float origin_y = impact.has_value() ? static_cast<float>(impact->y)
                                                          : frame.y + frame.height * MASK_CENTER;
                transform = {frame.width * scale,
                    frame.height * scale,
                    MASK_CENTER + (frame.x - origin_x) * scale,
                    MASK_CENTER + (frame.y - origin_y) * scale};
            }
            return this->pass->draw(this->state->opacity, transform);
        }
        class CrackSurface final : public effect::ICrackSurface {
          private:
            std::shared_ptr<platform::IShaderDataProvider> shaders;
            std::shared_ptr<CrackFrame> state;
            EffectSurface surface;

          public:
            explicit CrackSurface(std::shared_ptr<platform::IShaderDataProvider> shaders);
            foundation::Result<void, effect::EffectError> open(
                const effect::CrackSettings& settings, types::TextureData mask) override;
            foundation::Result<void, effect::EffectError> draw(float opacity,
                std::optional<effect::CrackImpact> impact,
                const effect::EffectTarget& target) override;
            bool hide() noexcept override;
            void close() noexcept override;
        };
        CrackSurface::CrackSurface(std::shared_ptr<platform::IShaderDataProvider> shaders)
            : shaders(std::move(shaders)) {
        }
        foundation::Result<void, effect::EffectError> CrackSurface::open(
            const effect::CrackSettings& settings, types::TextureData mask) {
            if (this->shaders == nullptr || this->state != nullptr) {
                return foundation::Error(effect::EffectError::StartFailed,
                    "Crack surface is unavailable or already open");
            }
            auto valid = validate_surface_input(settings, mask);
            if (valid.is_err()) {
                return valid;
            }
            auto state = std::make_shared<CrackFrame>();
            auto shared_mask = std::make_shared<types::TextureData>(std::move(mask));
            auto opened =
                this->surface.open([settings, shaders = this->shaders, shared_mask, state]() {
                    return std::make_unique<CrackContent>(settings, shaders, shared_mask, state);
                });
            if (opened.is_ok()) {
                this->state = std::move(state);
            }
            return opened;
        }
        foundation::Result<void, effect::EffectError> CrackSurface::draw(float opacity,
            std::optional<effect::CrackImpact> impact,
            const effect::EffectTarget& target) {
            if (this->state == nullptr) {
                return foundation::Error(
                    effect::EffectError::UpdateFailed, "Crack surface is not open");
            }
            if (!std::isfinite(opacity) || opacity < 0 || opacity > 1) {
                return foundation::Error(effect::EffectError::InvalidArgument,
                    "Crack opacity must be finite and between zero and one");
            }
            this->state->opacity = opacity;
            this->state->impact = impact;
            return this->surface.draw(target);
        }
        bool CrackSurface::hide() noexcept {
            return this->surface.hide();
        }
        void CrackSurface::close() noexcept {
            this->surface.close();
            this->state.reset();
        }
    } // namespace
    std::unique_ptr<effect::ICrackSurface> create_crack_surface(
        std::shared_ptr<platform::IShaderDataProvider> shaders) {
        return std::make_unique<CrackSurface>(std::move(shaders));
    }
} // namespace enishi::platform_impl
