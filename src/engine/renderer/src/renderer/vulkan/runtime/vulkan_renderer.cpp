#include <array>
#include <cstring>
#include <debug/log/logger.h>
#include <renderer/vulkan/command/vulkan_render_command_encoder.h>
#include <renderer/vulkan/mesh/vulkan_mesh_builder.h>
#include <renderer/vulkan/pipeline/vulkan_pipeline.h>
#include <renderer/vulkan/pipeline/vulkan_render_states.h>
#include <renderer/vulkan/runtime/vulkan_handles.h>
#include <renderer/vulkan/runtime/vulkan_render_resources.h>
#include <renderer/vulkan/runtime/vulkan_renderer.h>
#include <renderer/vulkan/shader/spirv_shader_reflection.h>
#include <renderer/vulkan/shadow/vulkan_shadow_resources.h>
#include <renderer/vulkan/view/vulkan_views.h>

namespace enishi::renderer::vulkan {
    VulkanRenderer::VulkanRenderer(std::shared_ptr<VulkanDevice> context)
        : resources(std::make_shared<VulkanRenderResources>(std::move(context)))
        , encoder(std::make_shared<VulkanRenderCommandEncoder>(this->resources)) {
    }
    VulkanRenderer::~VulkanRenderer() noexcept {
    }
    std::shared_ptr<VulkanDevice> VulkanRenderer::get_native_context() const noexcept {
        return this->resources->get_context();
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
        auto result = check_vk(vkCreateDescriptorSetLayout(this->resources->get_context()->device,
                                   &descriptor,
                                   nullptr,
                                   &this->resources->get_state().descriptors),
            "vkCreateDescriptorSetLayout");
        if (result.is_err()) {
            return result;
        }
        VkPipelineLayoutCreateInfo layout{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
        result = VulkanShadowResources::init_shadow_resources(this->resources->get_state());
        if (result.is_err()) {
            return result;
        }
        const std::array layouts{this->resources->get_state().descriptors,
            this->resources->get_state().shadow_descriptors};
        layout.setLayoutCount = static_cast<std::uint32_t>(layouts.size());
        layout.pSetLayouts = layouts.data();
        result = check_vk(vkCreatePipelineLayout(this->resources->get_context()->device,
                              &layout,
                              nullptr,
                              &this->resources->get_state().pipeline_layout),
            "vkCreatePipelineLayout");
        if (result.is_err()) {
            return result;
        }
        auto sampler = this->create_sampler(types::SamplerStateDescription::default_linear());
        if (sampler.is_err()) {
            return std::move(sampler).take_err();
        }
        this->resources->get_state().sampler =
            this->resources->get_state().samplers.at(sampler.unwrap());
        return {};
    }
    platform::RenderResult<types::RenderHandle> VulkanRenderer::create_viewport(
        const types::ViewportRect& viewport) {
        const auto handle = this->make_handle(types::RenderHandleType::ViewPort);
        this->resources->get_state().viewports.emplace(handle, viewport);
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
            constexpr std::uint32_t MATERIAL_UNIFORM_BINDING = 2;
            const auto expected_type = (resource.binding < FIRST_TEXTURE_BINDING ||
                                           resource.binding == SHADOW_UNIFORM_BINDING)
                                           ? types::ShaderInputResourceType::UniformBuffer
                                           : types::ShaderInputResourceType::Texture;
            if (resource.set != 0 || !(resource.binding < names.size()) ||
                (resource.binding != MATERIAL_UNIFORM_BINDING &&
                    resource.name != names[resource.binding]) ||
                resource.type != expected_type) {
                return foundation::Error(platform::RenderError::MakeError,
                    "SPIR-V descriptor does not match the model shader contract");
            }
        }
        auto [resource, stored] =
            this->resources->get_resource_accessor().make_shader_reflection(std::move(reflection));
        const auto handle = this->make_handle(types::RenderHandleType::ShaderReflection);
        (this->resources->get_handle_mapper())[handle].resource = resource;
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
            check_vk(vkCreateShaderModule(
                         this->resources->get_context()->device, &info, nullptr, &shader.module),
                "vkCreateShaderModule");
        if (result.is_err()) {
            return std::move(result).take_err();
        }
        const auto handle = this->make_handle(types::RenderHandleType::Shader);
        this->resources->get_state().shaders.emplace(handle, shader);
        return handle;
    }
    platform::RenderResult<types::RenderHandle> VulkanRenderer::create_texture(
        const types::TextureData& data) {
        auto image = this->resources->get_context()->make_texture(data);
        if (image.is_err()) {
            return std::move(image).take_err();
        }
        const auto handle = this->make_handle(types::RenderHandleType::Image);
        this->resources->get_state().textures.emplace(handle, std::move(image).unwrap());
        return handle;
    }
    platform::IRenderResourceAccessor* VulkanRenderer::get_resource_accessor() noexcept {
        return &this->resources->get_resource_accessor();
    }
    platform::IRenderResourceAccessor* const
    VulkanRenderer::get_resource_accessor() const noexcept {
        return &this->resources->get_resource_accessor();
    }
    const platform::IRenderHandleMapper* VulkanRenderer::get_handle_mapper() const noexcept {
        return &this->resources->get_handle_mapper();
    }
    platform::RenderResult<types::RenderHandle> VulkanRenderer::create_mesh(
        const types::ModelData& model, const std::vector<types::RenderHandle>& shaders) {
        return VulkanMeshBuilder::create_mesh(this->resources->get_state(),
            this->resources->get_resource_accessor(),
            this->resources->get_handle_mapper(),
            model,
            shaders);
    }
    platform::RenderResult<types::RenderHandle> VulkanRenderer::create_image(
        const types::ImageDescription& description) {
        return VulkanViews::create_image(
            this->resources->get_state(), this->resources->get_handle_mapper(), description);
    }
    platform::RenderResult<std::shared_ptr<platform::IRenderTargetView>>
    VulkanRenderer::create_render_target_view(
        types::RenderHandle image, const types::ImageViewDescription& description) {
        return VulkanViews::create_render_target_view(this->resources->get_state(),
            this->resources->get_resource_accessor(),
            this->resources->get_handle_mapper(),
            image,
            description);
    }
    platform::RenderResult<std::shared_ptr<platform::IDepthStencilView>>
    VulkanRenderer::create_depth_stencil_view(
        types::RenderHandle image, const types::ImageViewDescription& description) {
        return VulkanViews::create_depth_stencil_view(this->resources->get_state(),
            this->resources->get_resource_accessor(),
            this->resources->get_handle_mapper(),
            image,
            description);
    }
    platform::RenderResult<std::shared_ptr<platform::IShaderResourceView>>
    VulkanRenderer::create_shader_resource_view(
        types::RenderHandle image, const types::ImageViewDescription& description) {
        return VulkanViews::create_shader_resource_view(this->resources->get_state(),
            this->resources->get_resource_accessor(),
            this->resources->get_handle_mapper(),
            image,
            description);
    }
    platform::RenderResult<std::shared_ptr<platform::IUnorderedAccessView>>
    VulkanRenderer::create_unordered_access_view(
        types::RenderHandle image, const types::ImageViewDescription& description) {
        return VulkanViews::create_unordered_access_view(image, description);
    }
    platform::RenderResult<types::RenderHandle> VulkanRenderer::create_rasterizer(
        const types::RasterizerStateDescription& value) {
        return VulkanRenderStates::create_rasterizer(this->resources->get_state(),
            this->resources->get_resource_accessor(),
            this->resources->get_handle_mapper(),
            value);
    }
    platform::RenderResult<types::RenderHandle> VulkanRenderer::create_depth_stencil(
        const types::DepthStencilStateDescription& value) {
        return VulkanRenderStates::create_depth_stencil(this->resources->get_state(),
            this->resources->get_resource_accessor(),
            this->resources->get_handle_mapper(),
            value);
    }
    platform::RenderResult<types::RenderHandle> VulkanRenderer::create_blend(
        const types::BlendStateDescription& value) {
        return VulkanRenderStates::create_blend(this->resources->get_state(),
            this->resources->get_resource_accessor(),
            this->resources->get_handle_mapper(),
            value);
    }
    platform::RenderResult<types::RenderHandle> VulkanRenderer::create_sampler(
        const types::SamplerStateDescription& value) {
        return VulkanRenderStates::create_sampler(this->resources->get_state(),
            this->resources->get_resource_accessor(),
            this->resources->get_handle_mapper(),
            value);
    }
    types::RenderHandle VulkanRenderer::make_handle(types::RenderHandleType type) noexcept {
        return VulkanHandles::make(this->resources->get_handle_mapper(), type);
    }

    std::shared_ptr<platform::IRenderCommandEncoder>
    VulkanRenderer::get_command_encoder() const noexcept {
        return this->encoder;
    }
} // namespace enishi::renderer::vulkan
