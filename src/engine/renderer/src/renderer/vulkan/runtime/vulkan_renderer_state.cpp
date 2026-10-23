#include "vulkan_renderer_state.h"
namespace enishi::renderer::vulkan {
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
} // namespace enishi::renderer::vulkan
