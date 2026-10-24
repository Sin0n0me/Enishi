#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <algorithm>
#include <array>
#include <cstring>
#include <limits>
#include <renderer/vulkan/device/vulkan_device.h>
#include <renderer/vulkan/shadow/vulkan_shadow.h>
#include <renderer/vulkan/swapchain/vulkan_swapchain.h>
#include <renderer/vulkan/texture/vulkan_texture.h>

// Preserve VkResult and operation names at every failing Vulkan call.
#define ENISHI_VK_TRY(call)                                                                        \
    do {                                                                                           \
        auto checked = check_vk((call), #call);                                                    \
        if (checked.is_err()) {                                                                    \
            return std::move(checked).take_err();                                                  \
        }                                                                                          \
    } while (false)

namespace enishi::renderer::vulkan {
    VulkanDevice::VulkanDevice() = default;
    VulkanDevice::~VulkanDevice() {
        if (this->device != VK_NULL_HANDLE) {
            vkDeviceWaitIdle(this->device);
            this->swapchain.reset();
            vkDestroyCommandPool(this->device, this->command_pool, nullptr);
            vkDestroyDevice(this->device, nullptr);
        }
        if (this->instance != VK_NULL_HANDLE) {
            vkDestroySurfaceKHR(this->instance, this->surface, nullptr);
            if (this->messenger != VK_NULL_HANDLE) {
                const auto destroy = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
                    vkGetInstanceProcAddr(this->instance, "vkDestroyDebugUtilsMessengerEXT"));
                if (destroy != nullptr) {
                    destroy(this->instance, this->messenger, nullptr);
                }
            }
            vkDestroyInstance(this->instance, nullptr);
        }
    }
    platform::RenderResult<void> VulkanDevice::init(SDL_Window* window, bool validation) {
        if (window == nullptr) {
            return foundation::Error(
                platform::RenderError::MakeError, "Vulkan requires an SDL window");
        }
        auto result = this->make_instance(validation);
        if (result.is_err()) {
            return result;
        }
        if (!SDL_Vulkan_CreateSurface(window, this->instance, nullptr, &this->surface)) {
            return foundation::Error(platform::RenderError::MakeError, SDL_GetError());
        }
        result = this->select_device();
        if (result.is_err()) {
            return result;
        }
        result = this->make_commands();
        if (result.is_err()) {
            return result;
        }
        this->swapchain = std::make_unique<VulkanSwapchain>(*this, window, this->surface);
        return this->swapchain->init();
    }
    platform::RenderResult<std::shared_ptr<VulkanBuffer>> VulkanDevice::make_buffer(
        VkDeviceSize size, VkBufferUsageFlags usage) {
        if (size == 0) {
            return foundation::Error(
                platform::RenderError::MakeError, "Vulkan buffer size must be positive");
        }
        auto buffer = std::make_shared<VulkanBuffer>();
        buffer->owner = this->shared_from_this();
        buffer->size = size;
        VkBufferCreateInfo info{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
        info.size = size;
        info.usage = usage;
        info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        ENISHI_VK_TRY(vkCreateBuffer(this->device, &info, nullptr, &buffer->buffer));
        VkMemoryRequirements requirements{};
        vkGetBufferMemoryRequirements(this->device, buffer->buffer, &requirements);
        auto type = this->memory_type(requirements.memoryTypeBits,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        if (type.is_err()) {
            return std::move(type).take_err();
        }
        VkMemoryAllocateInfo allocation{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
        allocation.allocationSize = requirements.size;
        allocation.memoryTypeIndex = type.unwrap();
        ENISHI_VK_TRY(vkAllocateMemory(this->device, &allocation, nullptr, &buffer->memory));
        ENISHI_VK_TRY(vkBindBufferMemory(this->device, buffer->buffer, buffer->memory, 0));
        ENISHI_VK_TRY(vkMapMemory(this->device, buffer->memory, 0, size, 0, &buffer->mapped));
        return buffer;
    }
    platform::RenderResult<std::shared_ptr<VulkanImage>> VulkanDevice::make_texture(
        const types::TextureData& texture) {
        return VulkanTexture::make_texture(*this, texture);
    }
    platform::RenderResult<std::shared_ptr<VulkanShadowTarget>> VulkanDevice::make_shadow_target(
        VkExtent2D size) {
        return VulkanShadow::make_shadow_target(*this, size);
    }
    platform::RenderResult<void> VulkanDevice::begin_frame(glm::vec4 clear_color) {
        return this->swapchain->begin_frame(clear_color);
    }
    platform::RenderResult<void> VulkanDevice::begin_shadow_pass(
        const VulkanShadowTarget& target, float clear_depth) {
        return this->swapchain->begin_shadow_pass(target, clear_depth);
    }
    void VulkanDevice::begin_main_pass() {
        return this->swapchain->begin_main_pass();
    }
    void VulkanDevice::end_active_pass() {
        return this->swapchain->end_active_pass();
    }
    platform::RenderResult<void> VulkanDevice::end_frame(
        const std::shared_ptr<VulkanBuffer>& readback) {
        return this->swapchain->end_frame(readback);
    }
    platform::RenderResult<void> VulkanDevice::wait_idle() {
        return check_vk(vkDeviceWaitIdle(this->device), "vkDeviceWaitIdle");
    }
    std::uint32_t VulkanDevice::validation_error_count() const noexcept {
        return this->diagnostics.error_count();
    }
    platform::RenderResult<void> VulkanDevice::allocate_image(
        VkImage image, VkDeviceMemory& memory) {
        VkMemoryRequirements requirements{};
        vkGetImageMemoryRequirements(this->device, image, &requirements);
        auto type =
            this->memory_type(requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        if (type.is_err()) {
            return std::move(type).take_err();
        }
        VkMemoryAllocateInfo allocation{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
        allocation.allocationSize = requirements.size;
        allocation.memoryTypeIndex = type.unwrap();
        ENISHI_VK_TRY(vkAllocateMemory(this->device, &allocation, nullptr, &memory));
        return check_vk(vkBindImageMemory(this->device, image, memory, 0), "vkBindImageMemory");
    }
    platform::RenderResult<void> VulkanDevice::make_instance(bool validation) {
        Uint32 count = 0;
        const auto required = SDL_Vulkan_GetInstanceExtensions(&count);
        if (required == nullptr) {
            return foundation::Error(platform::RenderError::MakeError, SDL_GetError());
        }
        std::vector<const char*> extensions(required, required + count);
        std::vector<const char*> layers;
        if (validation) {
            extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
            layers.push_back("VK_LAYER_KHRONOS_validation");
        }
        VkApplicationInfo app{VK_STRUCTURE_TYPE_APPLICATION_INFO};
        app.pApplicationName = "Enishi";
        app.apiVersion = VK_API_VERSION_1_1;
        VkInstanceCreateInfo info{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
        info.pApplicationInfo = &app;
        info.enabledExtensionCount = static_cast<std::uint32_t>(extensions.size());
        info.ppEnabledExtensionNames = extensions.data();
        info.enabledLayerCount = static_cast<std::uint32_t>(layers.size());
        info.ppEnabledLayerNames = layers.data();
        ENISHI_VK_TRY(vkCreateInstance(&info, nullptr, &this->instance));
        if (validation) {
            auto debug = this->diagnostics.make_create_info();
            const auto create = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
                vkGetInstanceProcAddr(this->instance, "vkCreateDebugUtilsMessengerEXT"));
            if (create == nullptr) {
                return foundation::Error(
                    platform::RenderError::MakeError, "Vulkan debug utils unavailable");
            }
            ENISHI_VK_TRY(create(this->instance, &debug, nullptr, &this->messenger));
        }
        return {};
    }
    platform::RenderResult<void> VulkanDevice::select_device() {
        std::uint32_t count = 0;
        ENISHI_VK_TRY(vkEnumeratePhysicalDevices(this->instance, &count, nullptr));
        std::vector<VkPhysicalDevice> devices(count);
        ENISHI_VK_TRY(vkEnumeratePhysicalDevices(this->instance, &count, devices.data()));
        for (const auto candidate : devices) {
            VkPhysicalDeviceProperties properties{};
            vkGetPhysicalDeviceProperties(candidate, &properties);
            if (properties.apiVersion < VK_API_VERSION_1_1) {
                continue;
            }
            std::uint32_t extension_count = 0;
            ENISHI_VK_TRY(vkEnumerateDeviceExtensionProperties(
                candidate, nullptr, &extension_count, nullptr));
            std::vector<VkExtensionProperties> extensions(extension_count);
            ENISHI_VK_TRY(vkEnumerateDeviceExtensionProperties(
                candidate, nullptr, &extension_count, extensions.data()));
            if (!std::ranges::any_of(extensions, [](const auto& extension) {
                    return std::strcmp(extension.extensionName, VK_KHR_SWAPCHAIN_EXTENSION_NAME) ==
                           0;
                })) {
                continue;
            }
            std::uint32_t queue_count = 0;
            vkGetPhysicalDeviceQueueFamilyProperties(candidate, &queue_count, nullptr);
            std::vector<VkQueueFamilyProperties> families(queue_count);
            vkGetPhysicalDeviceQueueFamilyProperties(candidate, &queue_count, families.data());
            for (std::uint32_t index = 0; index < queue_count; ++index) {
                VkBool32 present = VK_FALSE;
                ENISHI_VK_TRY(vkGetPhysicalDeviceSurfaceSupportKHR(
                    candidate, index, this->surface, &present));
                if (present == VK_TRUE &&
                    (families[index].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0) {
                    this->physical = candidate;
                    this->queue_family = index;
                    break;
                }
            }
            if (this->physical != VK_NULL_HANDLE) {
                break;
            }
        }
        if (this->physical == VK_NULL_HANDLE) {
            return foundation::Error(
                platform::RenderError::MakeError, "No Vulkan 1.1 graphics/present device");
        }
        constexpr float PRIORITY = 1.0f;
        VkDeviceQueueCreateInfo queue_info{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
        queue_info.queueFamilyIndex = this->queue_family;
        queue_info.queueCount = 1;
        queue_info.pQueuePriorities = &PRIORITY;
        const char* extension = VK_KHR_SWAPCHAIN_EXTENSION_NAME;
        VkDeviceCreateInfo info{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
        VkPhysicalDeviceFeatures supported{};
        vkGetPhysicalDeviceFeatures(this->physical, &supported);
        VkPhysicalDeviceFeatures enabled{};
        enabled.textureCompressionBC = supported.textureCompressionBC;
        enabled.samplerAnisotropy = supported.samplerAnisotropy;
        info.pEnabledFeatures = &enabled;
        info.queueCreateInfoCount = 1;
        info.pQueueCreateInfos = &queue_info;
        info.enabledExtensionCount = 1;
        info.ppEnabledExtensionNames = &extension;
        ENISHI_VK_TRY(vkCreateDevice(this->physical, &info, nullptr, &this->device));
        vkGetDeviceQueue(this->device, this->queue_family, 0, &this->queue);
        return {};
    }
    platform::RenderResult<void> VulkanDevice::make_commands() {
        VkCommandPoolCreateInfo pool{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
        pool.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        pool.queueFamilyIndex = this->queue_family;
        ENISHI_VK_TRY(vkCreateCommandPool(this->device, &pool, nullptr, &this->command_pool));
        VkCommandBufferAllocateInfo allocation{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
        allocation.commandPool = this->command_pool;
        allocation.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocation.commandBufferCount = 1;
        ENISHI_VK_TRY(vkAllocateCommandBuffers(this->device, &allocation, &this->command));
        return {};
    }
    platform::RenderResult<std::uint32_t> VulkanDevice::memory_type(
        std::uint32_t mask, VkMemoryPropertyFlags flags) const {
        VkPhysicalDeviceMemoryProperties memory{};
        vkGetPhysicalDeviceMemoryProperties(this->physical, &memory);
        for (std::uint32_t index = 0; index < memory.memoryTypeCount; ++index) {
            if ((mask & (1u << index)) != 0 &&
                (memory.memoryTypes[index].propertyFlags & flags) == flags) {
                return index;
            }
        }
        return foundation::Error(
            platform::RenderError::MakeError, "No compatible Vulkan memory type");
    }
} // namespace enishi::renderer::vulkan
