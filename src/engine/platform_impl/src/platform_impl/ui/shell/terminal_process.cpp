#include "terminal_process.h"
#include <SDL3/SDL.h>
#include <memory>
namespace enishi::platform_impl {
    namespace {
        struct Properties {
            SDL_PropertiesID value = SDL_CreateProperties();
            ~Properties();
        };
        Properties::~Properties() {
            if (this->value != 0) {
                SDL_DestroyProperties(this->value);
            }
        }
    } // namespace
    std::expected<void, std::string> launch_terminal_process(const char* const* arguments) {
        const Properties properties;
        if (properties.value == 0 ||
            !SDL_SetPointerProperty(properties.value,
                SDL_PROP_PROCESS_CREATE_ARGS_POINTER,
                const_cast<const char**>(arguments)) ||
            !SDL_SetBooleanProperty(
                properties.value, SDL_PROP_PROCESS_CREATE_BACKGROUND_BOOLEAN, true)) {
            return std::unexpected(std::string(SDL_GetError()));
        }
        const std::unique_ptr<SDL_Process, decltype(&SDL_DestroyProcess)> process(
            SDL_CreateProcessWithProperties(properties.value), SDL_DestroyProcess);
        if (process == nullptr) {
            return std::unexpected(std::string(SDL_GetError()));
        }
        return {};
    }
} // namespace enishi::platform_impl
