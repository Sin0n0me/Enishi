#pragma once
#include "vulkan_device.h"
#include <array>
#include <engine_types/renderer/mesh/mesh_data.h>
#include <map>
#include <platform/renderer/shader/interface_shader_reflection.h>
#include <unordered_map>

namespace enishi::renderer::vulkan {
    using VulkanUniformMap = std::unordered_map<std::string, std::shared_ptr<VulkanBuffer>>;
    struct VulkanMaterial {
        types::DrawBinding draw;
        VkDescriptorSet descriptors = VK_NULL_HANDLE;
        VulkanUniformMap uniforms;
        std::vector<std::shared_ptr<VulkanImage>> textures;
    };
    struct VulkanMesh {
        std::shared_ptr<VulkanDevice> owner;
        std::shared_ptr<VulkanBuffer> vertices;
        std::shared_ptr<VulkanBuffer> indices;
        VkIndexType index_type = VK_INDEX_TYPE_UINT32;
        VkDescriptorPool pool = VK_NULL_HANDLE;
        std::vector<VkDescriptorPool> sampler_pools;
        std::unordered_map<VkSampler, std::vector<VkDescriptorSet>> sampler_descriptors;
        VulkanUniformMap uniforms;
        std::vector<VulkanMaterial> materials;
        ~VulkanMesh();
    };
    struct VulkanShader {
        VkShaderModule module = VK_NULL_HANDLE;
        types::ShaderKind kind = types::ShaderKind::Unknown;
    };
    class VulkanRendererState final {
      public:
        std::shared_ptr<VulkanDevice> context;
        VkDescriptorSetLayout descriptors = VK_NULL_HANDLE;
        VkDescriptorSetLayout shadow_descriptors = VK_NULL_HANDLE;
        VkSampler shadow_sampler = VK_NULL_HANDLE;
        std::shared_ptr<VulkanShadowTarget> fallback_shadow;
        std::shared_ptr<VulkanShadowTarget> sampled_shadow;
        std::shared_ptr<VulkanShadowTarget> active_shadow;
        std::unordered_map<types::RenderHandle, std::shared_ptr<VulkanShadowTarget>> shadow_targets;
        VkPipelineLayout pipeline_layout = VK_NULL_HANDLE;
        VkSampler sampler = VK_NULL_HANDLE;
        std::unordered_map<types::RenderHandle, VkSampler> samplers;
        std::unordered_map<types::RenderHandle, std::shared_ptr<VulkanMesh>> meshes;
        std::unordered_map<types::RenderHandle, VulkanShader> shaders;
        std::unordered_map<types::RenderHandle, std::shared_ptr<VulkanImage>> textures;
        std::unordered_map<types::RenderHandle, types::ImageDescription> images;
        std::unordered_map<types::RenderHandle, types::RasterizerStateDescription> rasterizers;
        std::unordered_map<types::RenderHandle, types::DepthStencilStateDescription> depths;
        std::unordered_map<types::RenderHandle, types::BlendStateDescription> blends;
        std::unordered_map<types::RenderHandle, types::ViewportRect> viewports;
        std::map<std::array<std::uint32_t, 6>, VkPipeline> pipelines;
        types::RenderHandle main_viewport;
        types::RenderHandle vertex_shader;
        types::RenderHandle pixel_shader;
        types::RenderHandle rasterizer;
        types::RenderHandle depth;
        types::RenderHandle blend;
        bool failed = false;
        explicit VulkanRendererState(std::shared_ptr<VulkanDevice> context);
        ~VulkanRendererState();
    };
} // namespace enishi::renderer::vulkan
