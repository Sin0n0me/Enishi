#include "vulkan_mesh.h"
namespace enishi::renderer::vulkan {
    VulkanMesh::~VulkanMesh() {
        if (this->owner != nullptr) {
            for (const auto pool : this->sampler_pools) {
                vkDestroyDescriptorPool(this->owner->device, pool, nullptr);
            }
            vkDestroyDescriptorPool(this->owner->device, this->pool, nullptr);
        }
    }
} // namespace enishi::renderer::vulkan
