#pragma once
#include "shader_input_info.h"
#include <cstdint>
#include <string_view>

namespace enishi::renderer {
    class IShaderInputReflection {
      public:
        virtual ~IShaderInputReflection(void) noexcept = default;

        [[nodiscard]]
        virtual std::uint32_t get_input_count(void) const noexcept = 0;

        [[nodiscard]]
        virtual ShaderInputInfo get_input(const std::uint32_t index) const noexcept = 0;
    };
} // namespace enishi::renderer