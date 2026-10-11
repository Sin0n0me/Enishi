#include "spirv_shader_reflection.h"
#include <spirv_reflect.h>
namespace enishi::renderer::vulkan {
    namespace {
        class Module final {
          public:
            SpvReflectShaderModule data{};
            ~Module();
        };
        Module::~Module() {
            spvReflectDestroyShaderModule(&this->data);
        }
    } // namespace
    foundation::VoidResult<platform::RenderError> SpirvShaderReflection::load(
        const types::ShaderData& shader) noexcept {
        if (shader.binary_type != types::ShaderBinaryType::SPIR_V) {
            return foundation::Error(
                platform::RenderError::MakeError, "Vulkan requires SPIR-V shaders");
        }
        Module module;
        const auto result =
            spvReflectCreateShaderModule(shader.code.size(), shader.code.data(), &module.data);
        if (result != SPV_REFLECT_RESULT_SUCCESS) {
            return foundation::Error(platform::RenderError::MakeError,
                std::format("SPIR-V reflection failed ({})", static_cast<int>(result)));
        }
        this->hash = shader.hash();
        this->input = {};
        if (module.data.shader_stage == SPV_REFLECT_SHADER_STAGE_VERTEX_BIT) {
            this->kind = types::ShaderKind::Vertex;
        } else if (module.data.shader_stage == SPV_REFLECT_SHADER_STAGE_FRAGMENT_BIT) {
            this->kind = types::ShaderKind::Pixel;
        } else {
            return foundation::Error(
                platform::RenderError::MakeError, "Vulkan supports vertex/fragment shaders");
        }
        for (std::uint32_t index = 0; index < module.data.input_variable_count; ++index) {
            const auto& variable = *module.data.input_variables[index];
            if ((variable.decoration_flags & SPV_REFLECT_DECORATION_BUILT_IN) != 0) {
                continue;
            }
            auto type = types::ShaderInputValueType::Float;
            if ((variable.type_description->type_flags & SPV_REFLECT_TYPE_FLAG_INT) != 0) {
                type = variable.numeric.scalar.signedness != 0
                           ? types::ShaderInputValueType::SignedInteger
                           : types::ShaderInputValueType::UnsignedInteger;
            }
            this->input.add_layout({.name = variable.name == nullptr ? "" : variable.name,
                .value_type = type,
                .location = variable.location,
                .array_size = 1,
                .component = 0,
                .component_count = (std::max)(1u, variable.numeric.vector.component_count)});
        }
        for (std::uint32_t index = 0; index < module.data.descriptor_binding_count; ++index) {
            const auto& binding = module.data.descriptor_bindings[index];
            const bool uniform =
                binding.descriptor_type == SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            if (!uniform &&
                binding.descriptor_type != SPV_REFLECT_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER) {
                return foundation::Error(
                    platform::RenderError::MakeError, "Unsupported Vulkan descriptor type");
            }
            const auto name = uniform ? binding.type_description->type_name : binding.name;
            if (name == nullptr || binding.set != 0 || binding.count != 1) {
                return foundation::Error(platform::RenderError::MakeError,
                    "Vulkan expects named scalar descriptors in set zero");
            }
            this->input.add_resource({.name = name,
                .type = uniform ? types::ShaderInputResourceType::UniformBuffer
                                : types::ShaderInputResourceType::Texture,
                .dimension = uniform ? types::ShaderInputResourceDimension::Buffer
                                     : types::ShaderInputResourceDimension::Texture2D,
                .read_only = types::ShaderInputResourceAccess::ReadOnly,
                .set = binding.set,
                .binding = binding.binding,
                .array_size = binding.count});
        }
        return {};
    }
    SpirvShaderReflection::SpirvShaderReflection() noexcept
        : kind(types::ShaderKind::Unknown)
        , hash(0) {
    }
    void SpirvShaderInputReflection::add_layout(types::ShaderInputLayout layout) {
        this->layouts.emplace_back(std::move(layout));
    }
    void SpirvShaderInputReflection::add_resource(types::ShaderInputResource resource) {
        this->resources.emplace_back(std::move(resource));
    }
    std::uint32_t SpirvShaderInputReflection::get_input_layout_count(void) const noexcept {
        return static_cast<std::uint32_t>(this->layouts.size());
    }
    foundation::Option<types::ShaderInputLayout> SpirvShaderInputReflection::get_input_layout(
        const std::uint32_t index) const noexcept {
        return index < this->layouts.size()
                   ? foundation::Option<types::ShaderInputLayout>(this->layouts[index])
                   : foundation::Option<types::ShaderInputLayout>();
    }
    std::vector<types::ShaderInputLayout> SpirvShaderInputReflection::get_input_layouts(
        void) const noexcept {
        return this->layouts;
    }
    std::uint32_t SpirvShaderInputReflection::get_input_resource_count(void) const noexcept {
        return static_cast<std::uint32_t>(this->resources.size());
    }
    foundation::Option<types::ShaderInputResource> SpirvShaderInputReflection::get_input_resource(
        const std::uint32_t index) const noexcept {
        return index < this->resources.size()
                   ? foundation::Option<types::ShaderInputResource>(this->resources[index])
                   : foundation::Option<types::ShaderInputResource>();
    }
    std::vector<types::ShaderInputResource> SpirvShaderInputReflection::get_input_resources(
        void) const noexcept {
        return this->resources;
    }
    foundation::Option<types::ShaderInputResource>
    SpirvShaderInputReflection::resolve_input_resource(
        const foundation::UTF8& name) const noexcept {
        for (const auto& resource : this->resources) {
            if (resource.name == name) {
                return resource;
            }
        }
        return {};
    }

    const platform::IShaderInputReflection* SpirvShaderReflection::get_shader_input_reflection(
        void) const {
        return &this->input;
    }
    types::ShaderKind SpirvShaderReflection::get_shader_kind(void) const {
        return this->kind;
    }
    std::size_t SpirvShaderReflection::get_shader_hash(void) const {
        return this->hash;
    }
} // namespace enishi::renderer::vulkan
