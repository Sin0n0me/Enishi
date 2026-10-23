#include "opengl40_handles.h"

namespace enishi::renderer::opengl {
    types::RenderHandle OpenGL40Handles::make_handle(
        RenderHandleMapper& handle_mapper, const types::RenderHandleType type) noexcept {
        auto handle = handle_mapper.make(type, {});
        handle_mapper[handle].resource = handle.id;
        return handle;
    }
} // namespace enishi::renderer::opengl
