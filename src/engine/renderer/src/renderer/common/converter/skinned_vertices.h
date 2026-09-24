#pragma once
#include <cstddef>
#include <engine_types/assets/model/model_data.h>
#include <foundation/result/result.h>
#include <optional>
#include <renderer/errors/errors.h>
#include <string_view>

namespace enishi::renderer {
    // Shared byte layout for all model passes in GLSL and HLSL.
    struct SkinnedVertex {
        glm::vec3 position{};
        glm::vec3 normal{};
        glm::vec2 uv{};
        glm::u32vec4 bones{};
        glm::vec4 weights{};
        float edge{};
        std::uint32_t method{};
        glm::vec3 center{};
        glm::vec3 anchor0{};
        glm::vec3 anchor1{};
    };

    static_assert(sizeof(SkinnedVertex) == 108);
    static_assert(offsetof(SkinnedVertex, bones) == 32);
    static_assert(offsetof(SkinnedVertex, weights) == 48);
    static_assert(offsetof(SkinnedVertex, method) == 68);
    static_assert(offsetof(SkinnedVertex, center) == 72);

    [[nodiscard]] foundation::Result<std::vector<SkinnedVertex>, RendererError>
    make_skinned_vertices(const types::ModelData& model);
    [[nodiscard]] std::optional<std::uint32_t> skinned_vertex_offset(std::string_view semantic);
} // namespace enishi::renderer
