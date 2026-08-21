#pragma once
#include "interface_shader_input_reflection.h"
#include "interface_shader_reflection.h"
#include <foundation/option/option.h>
#include <memory>
#include <platform/renderer/interface_image_view.h>

namespace enishi::renderer {
    class IShaderAccessor {
      public:
        using ShaderReflection = std::shared_ptr<IShaderReflection>;
        // using Shader = std::shared_ptr<>;

      public:
        virtual ~IShaderAccessor(void) noexcept = default;

        [[nodiscard]] virtual std::tuple<types::HandleId, ShaderReflection&> make_shader_reflection(
            ShaderReflection&& shader_reflection) noexcept = 0;

        [[nodiscard]] virtual foundation::Option<ShaderReflection&> get_shader_reflection(
            const types::HandleId handle) noexcept = 0;
        [[nodiscard]] virtual foundation::Option<const ShaderReflection&> get_shader_reflection(
            const types::HandleId handle) const noexcept = 0;

        [[nodiscard]] virtual void remove_shader_reflection(
            const types::HandleId handle) noexcept = 0;
    };
} // namespace enishi::renderer