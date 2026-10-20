#include "vulkan_renderer.h"
#include "spirv_shader_reflection.h"
#include <array>
#include <cstring>
#include <foundation/log/logger.h>

namespace enishi::renderer::vulkan {
    VulkanMesh::~VulkanMesh() {
        if (this->owner != nullptr) {
            for (const auto pool : this->sampler_pools) {
                vkDestroyDescriptorPool(this->owner->device, pool, nullptr);
            }
            vkDestroyDescriptorPool(this->owner->device, this->pool, nullptr);
        }
    }
    VulkanRendererState::VulkanRendererState(std::shared_ptr<VulkanDevice> context)
        : context(std::move(context)) {
    }
    VulkanRendererState::~VulkanRendererState() {
        const auto device = this->context->device;
        for (const auto& [key, pipeline] : this->pipelines) {
            vkDestroyPipeline(device, pipeline, nullptr);
        }
        for (const auto& [handle, shader] : this->shaders) {
            vkDestroyShaderModule(device, shader.module, nullptr);
        }
        this->active_shadow.reset();
        this->sampled_shadow.reset();
        this->shadow_targets.clear();
        this->fallback_shadow.reset();
        vkDestroySampler(device, this->shadow_sampler, nullptr);
        vkDestroyDescriptorSetLayout(device, this->shadow_descriptors, nullptr);
        vkDestroyPipelineLayout(device, this->pipeline_layout, nullptr);
        vkDestroyDescriptorSetLayout(device, this->descriptors, nullptr);
        for (const auto& [handle, sampler] : this->samplers) {
            vkDestroySampler(device, sampler, nullptr);
        }
    }
    VulkanRenderer::VulkanRenderer(std::shared_ptr<VulkanDevice> context)
        : context(std::move(context))
        , handle_mapper(std::make_unique<RenderHandleMapper>())
        , resource_accessor(std::make_unique<VulkanResourceAccessor>())
        , state(std::make_unique<VulkanRendererState>(this->context)) {
    }
    VulkanRenderer::~VulkanRenderer() noexcept {
        auto result = this->context->wait_idle();
        if (result.is_err()) {
            foundation::Logger::error(result.unwrap_err().get_message());
        }
    }
    std::shared_ptr<VulkanDevice> VulkanRenderer::get_native_context() const noexcept {
        return this->context;
    }
    types::RenderHandle VulkanRenderer::make_handle(types::RenderHandleType type) noexcept {
        auto handle = this->handle_mapper->make(type, {});
        (*this->handle_mapper)[handle].resource = handle.id;
        return handle;
    }
    platform::RenderResult<void> VulkanRenderer::init_resources() {
        // Set zero: Camera, Bones, Material, base/sphere/toon textures, ShadowMap.
        constexpr std::uint32_t UNIFORM_COUNT = 3;
        constexpr std::uint32_t DESCRIPTOR_COUNT = 7;
        constexpr std::uint32_t SHADOW_UNIFORM_BINDING = 6;
        std::array<VkDescriptorSetLayoutBinding, DESCRIPTOR_COUNT> bindings{};
        for (std::uint32_t index = 0; index < bindings.size(); ++index) {
            bindings[index].binding = index;
            bindings[index].descriptorCount = 1;
            bindings[index].descriptorType =
                (index < UNIFORM_COUNT || index == SHADOW_UNIFORM_BINDING)
                    ? VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER
                    : VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            bindings[index].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
        }
        VkDescriptorSetLayoutCreateInfo descriptor{
            VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
        descriptor.bindingCount = static_cast<std::uint32_t>(bindings.size());
        descriptor.pBindings = bindings.data();
        auto result =
            check_vk(vkCreateDescriptorSetLayout(
                         this->context->device, &descriptor, nullptr, &this->state->descriptors),
                "vkCreateDescriptorSetLayout");
        if (result.is_err()) {
            return result;
        }
        VkPipelineLayoutCreateInfo layout{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
        result = this->init_shadow_resources();
        if (result.is_err()) {
            return result;
        }
        const std::array layouts{this->state->descriptors, this->state->shadow_descriptors};
        layout.setLayoutCount = static_cast<std::uint32_t>(layouts.size());
        layout.pSetLayouts = layouts.data();
        result =
            check_vk(vkCreatePipelineLayout(
                         this->context->device, &layout, nullptr, &this->state->pipeline_layout),
                "vkCreatePipelineLayout");
        if (result.is_err()) {
            return result;
        }
        auto sampler = this->create_sampler(types::SamplerStateDescription::default_linear());
        if (sampler.is_err()) {
            return std::move(sampler).take_err();
        }
        this->state->sampler = this->state->samplers.at(sampler.unwrap());
        return {};
    }
    platform::RenderResult<types::RenderHandle> VulkanRenderer::create_viewport(
        const types::ViewportRect& viewport) {
        const auto handle = this->make_handle(types::RenderHandleType::ViewPort);
        this->state->viewports.emplace(handle, viewport);
        return handle;
    }
    platform::RenderResult<types::RenderHandle> VulkanRenderer::create_shader_reflection(
        const types::ShaderData& data) {
        auto reflection = std::make_shared<SpirvShaderReflection>();
        auto result = reflection->load(data);
        if (result.is_err()) {
            return std::move(result).take_err();
        }
        const std::array<std::string_view, 7> names{"Camera",
            "Bones",
            "Material",
            "model_texture",
            "sphere_texture",
            "toon_texture",
            "ShadowMap"};
        for (const auto& resource :
            reflection->get_shader_input_reflection()->get_input_resources()) {
            constexpr std::uint32_t SHADOW_SET = 1;
            if (resource.set == SHADOW_SET && resource.binding == 0 &&
                resource.name == "shadow_texture" &&
                resource.type == types::ShaderInputResourceType::Texture) {
                continue;
            }
            constexpr std::uint32_t SHADOW_UNIFORM_BINDING = 6;
            constexpr std::uint32_t FIRST_TEXTURE_BINDING = 3;
            const auto expected_type = (resource.binding < FIRST_TEXTURE_BINDING ||
                                           resource.binding == SHADOW_UNIFORM_BINDING)
                                           ? types::ShaderInputResourceType::UniformBuffer
                                           : types::ShaderInputResourceType::Texture;
            if (resource.set != 0 || !(resource.binding < names.size()) ||
                resource.name != names[resource.binding] || resource.type != expected_type) {
                return foundation::Error(platform::RenderError::MakeError,
                    "SPIR-V descriptor does not match the model shader contract");
            }
        }
        auto [resource, stored] =
            this->resource_accessor->make_shader_reflection(std::move(reflection));
        const auto handle = this->make_handle(types::RenderHandleType::ShaderReflection);
        (*this->handle_mapper)[handle].resource = resource;
        return handle;
    }
    platform::RenderResult<types::RenderHandle>
    VulkanRenderer::create_vertex_layout_from_shader_data(const types::ShaderData& data) {
        SpirvShaderReflection reflection;
        auto result = reflection.load(data);
        if (result.is_err()) {
            return std::move(result).take_err();
        }
        constexpr std::array<std::uint32_t, 9> COMPONENTS{3, 3, 2, 4, 4, 1, 3, 3, 3};
        constexpr std::uint32_t BONE_LOCATION = 3;
        constexpr std::uint32_t METHOD_LOCATION = 5;
        for (const auto& input : reflection.get_shader_input_reflection()->get_input_layouts()) {
            const auto type = input.location == BONE_LOCATION || input.location == METHOD_LOCATION
                                  ? types::ShaderInputValueType::UnsignedInteger
                                  : types::ShaderInputValueType::Float;
            if (!(input.location < COMPONENTS.size()) ||
                input.component_count != COMPONENTS[input.location] || input.value_type != type) {
                return foundation::Error(platform::RenderError::MakeError,
                    "Incompatible Vulkan model vertex input layout");
            }
        }
        return this->make_handle(types::RenderHandleType::VertexLayout);
    }
    platform::RenderResult<std::unique_ptr<platform::IPipelineLayout>>
    VulkanRenderer::create_vertex_layout(
        const types::VertexLayout&, const types::RenderHandle&, const types::RenderHandle&) {
        return foundation::Error(
            platform::RenderError::MakeError, "Use reflected Vulkan vertex layouts");
    }
    platform::RenderResult<types::RenderHandle> VulkanRenderer::create_shader(
        types::ShaderKind kind, const types::ShaderData& data) {
        SpirvShaderReflection reflection;
        auto checked = reflection.load(data);
        if (checked.is_err()) {
            return std::move(checked).take_err();
        }
        if (reflection.get_shader_kind() != kind || data.code.size() % sizeof(std::uint32_t) != 0) {
            return foundation::Error(
                platform::RenderError::MakeError, "SPIR-V stage or byte size mismatch");
        }
        std::vector<std::uint32_t> words(data.code.size() / sizeof(std::uint32_t));
        std::memcpy(words.data(), data.code.data(), data.code.size());
        VkShaderModuleCreateInfo info{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
        info.codeSize = data.code.size();
        info.pCode = words.data();
        VulkanShader shader{.kind = kind};
        auto result =
            check_vk(vkCreateShaderModule(this->context->device, &info, nullptr, &shader.module),
                "vkCreateShaderModule");
        if (result.is_err()) {
            return std::move(result).take_err();
        }
        const auto handle = this->make_handle(types::RenderHandleType::Shader);
        this->state->shaders.emplace(handle, shader);
        return handle;
    }
    platform::RenderResult<types::RenderHandle> VulkanRenderer::create_texture(
        const types::TextureData& data) {
        auto image = this->context->make_texture(data);
        if (image.is_err()) {
            return std::move(image).take_err();
        }
        const auto handle = this->make_handle(types::RenderHandleType::Image);
        this->state->textures.emplace(handle, std::move(image).unwrap());
        return handle;
    }
    platform::IRenderResourceAccessor* VulkanRenderer::get_resource_accessor() noexcept {
        return this->resource_accessor.get();
    }
    platform::IRenderResourceAccessor* const
    VulkanRenderer::get_resource_accessor() const noexcept {
        return this->resource_accessor.get();
    }
    const platform::IRenderHandleMapper* VulkanRenderer::get_handle_mapper() const noexcept {
        return this->handle_mapper.get();
    }
} // namespace enishi::renderer::vulkan
