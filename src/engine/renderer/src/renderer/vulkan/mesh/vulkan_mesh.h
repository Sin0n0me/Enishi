#pragma once
#include <engine_types/renderer/mesh/mesh_data.h>
#include <renderer/vulkan/device/vulkan_device.h>
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
} // namespace enishi::renderer::vulkan
