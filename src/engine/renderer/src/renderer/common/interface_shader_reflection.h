#pragma once
#include "interface_shader_input_reflection.h"
#include <engine_types/assets/shader/shader_data.h>
#include <foundation/option/option.h>
#include <foundation/result/result.h>
#include <foundation/str/str.h>
#include <memory>


namespace enishi::renderer {
    template <typename E> class IShaderReflection {
      public:
        virtual ~IShaderReflection(void) noexcept = default;

        virtual foundation::VoidResult<E> load(
            std::shared_ptr<types::ShaderData> shader_data) noexcept = 0;

        virtual std::shared_ptr<types::ShaderData> get_shader_data(void) const = 0;

        virtual const IShaderInputReflection* get_shader_input_reflection(void) const = 0;
    };
} // namespace enishi::renderer