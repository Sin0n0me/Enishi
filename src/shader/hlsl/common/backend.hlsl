#ifndef ENISHI_SHADER_BACKEND
#define ENISHI_SHADER_BACKEND
#ifdef ENISHI_SPIRV
#define ENISHI_BINDING(slot, group) [[vk::binding(slot, group)]]
#define ENISHI_LOCATION(slot) [[vk::location(slot)]]
#define ENISHI_COMBINED [[vk::combinedImageSampler]]
#else
#define ENISHI_BINDING(binding, set)
#define ENISHI_LOCATION(location)
#define ENISHI_COMBINED
#endif
#ifdef ENISHI_VULKAN
#define ENISHI_BONE_CAPACITY 1024
#else
#define ENISHI_BONE_CAPACITY 512
#endif
#endif
