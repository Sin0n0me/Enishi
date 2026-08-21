#pragma once
#include "interface_shader_input_reflection.h"
#include <engine_types/assets/shader/shader_data.h>
#include <engine_types/assets/shader/shader_kind.h>
#include <foundation/option/option.h>
#include <foundation/result/result.h>
#include <foundation/str/str.h>
#include <memory>
#include <renderer/errors/errors.h>

namespace enishi::renderer {
    class IShaderReflection {
      public:
        virtual ~IShaderReflection(void) noexcept = default;

        virtual foundation::VoidResult<RendererError> load(
            const types::ShaderData& shader_data) noexcept = 0;

        virtual const IShaderInputReflection* get_shader_input_reflection(void) const = 0;

        virtual types::ShaderKind get_shader_kind(void) const = 0;

        virtual std::size_t get_shader_hash(void) const = 0;
    };
} // namespace enishi::renderer