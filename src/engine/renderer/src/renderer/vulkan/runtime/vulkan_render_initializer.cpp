#include <renderer/vulkan/runtime/vulkan_render_initializer.h>
namespace enishi::renderer::vulkan {
    platform::RenderResult<std::shared_ptr<VulkanRenderer>> VulkanRenderInitializer::init(
        SDL_Window* window, bool validation) {
        auto context = std::make_shared<VulkanDevice>();
        auto result = context->init(window, validation);
        if (result.is_err()) {
            return std::move(result).take_err();
        }
        auto renderer = std::make_shared<VulkanRenderer>(context);
        result = renderer->init_resources();
        if (result.is_err()) {
            return std::move(result).take_err();
        }
        return renderer;
    }
} // namespace enishi::renderer::vulkan
