#include "effect_display_target.h"
#include <string>
namespace enishi::platform_impl {
    foundation::Result<std::optional<SDL_DisplayID>, effect::EffectError> resolve_effect_display(
        const effect::EffectTarget& target) {
        switch (target.kind) {
            case effect::EffectTargetKind::AllDisplays:
                return std::optional<SDL_DisplayID>{};
            case effect::EffectTargetKind::DisplayAtPoint: {
                const SDL_Point point{target.x, target.y};
                const auto display = SDL_GetDisplayForPoint(&point);
                if (display == 0) {
                    return foundation::Error(effect::EffectError::UpdateFailed,
                        std::string{"Cannot resolve effect target display: "} + SDL_GetError());
                }
                return std::optional<SDL_DisplayID>{display};
            }
            default:
                return foundation::Error(
                    effect::EffectError::InvalidArgument, "Unknown effect target");
        }
    }
} // namespace enishi::platform_impl
