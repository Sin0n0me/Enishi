#pragma once
#include <cstdint>
#include <engine_types/renderer/render_data.h>
#include <engine_types/renderer/shader/shader_input_info.h>
#include <foundation/option/option.h>
#include <foundation/str/str.h>
#include <string_view>
#include <vector>

namespace enishi::platform {
    class IShaderInputReflection {
      public:
        virtual ~IShaderInputReflection(void) noexcept = default;

        [[nodiscard]]
        virtual std::uint32_t get_input_layout_count(void) const noexcept = 0;

        [[nodiscard]]
        virtual foundation::Option<types::ShaderInputLayout> get_input_layout(
            const std::uint32_t index) const noexcept = 0;

        [[nodiscard]]
        virtual std::vector<types::ShaderInputLayout> get_input_layouts(void) const noexcept = 0;

        [[nodiscard]]
        virtual std::uint32_t get_input_resource_count(void) const noexcept = 0;

        [[nodiscard]]
        virtual foundation::Option<types::ShaderInputResource> get_input_resource(
            const std::uint32_t index) const noexcept = 0;

        [[nodiscard]]
        virtual std::vector<types::ShaderInputResource> get_input_resources(
            void) const noexcept = 0;

        [[nodiscard]]
        virtual foundation::Option<types::ShaderInputResource> resolve_input_resource(
            const foundation::UTF8& name) const noexcept = 0;
    };
} // namespace enishi::platform