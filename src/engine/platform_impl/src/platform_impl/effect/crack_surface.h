#pragma once
#include <effect/crack/interface_crack_surface.h>
#include <memory>
#include <platform/asset/interface_shader_data_provider.h>
namespace enishi::platform_impl {
    [[nodiscard]] std::unique_ptr<effect::ICrackSurface> create_crack_surface(
        std::shared_ptr<platform::IShaderDataProvider> shaders);
}
