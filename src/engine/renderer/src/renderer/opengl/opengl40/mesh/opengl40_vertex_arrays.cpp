#include "opengl40_vertex_arrays.h"
#include <cstddef>
#include <renderer/common/converter/skinned_vertices.h>
#include <renderer/opengl/opengl40/support/opengl40_errors.h>
namespace enishi::renderer::opengl {
    namespace {
        constexpr std::uint32_t NO_GL_OBJECT = 0;
        constexpr std::uint32_t RESOURCE_COUNT = 1;
        constexpr std::uint32_t POSITION_ATTRIBUTE = 0;
        constexpr std::uint32_t NORMAL_ATTRIBUTE = 1;
        constexpr std::uint32_t TEXTURE_COORDINATE_ATTRIBUTE = 2;
        constexpr std::uint32_t BLEND_INDEX_ATTRIBUTE = 3;
        constexpr std::uint32_t BLEND_WEIGHT_ATTRIBUTE = 4;
        constexpr std::int32_t POSITION_COMPONENT_COUNT = 3;
        constexpr std::int32_t NORMAL_COMPONENT_COUNT = 3;
        constexpr std::int32_t TEXTURE_COORDINATE_COMPONENT_COUNT = 2;
        constexpr std::int32_t BLEND_INDEX_COMPONENT_COUNT = 4;
        constexpr std::int32_t BLEND_WEIGHT_COMPONENT_COUNT = 4;
        constexpr std::uint32_t SKINNING_METHOD_ATTRIBUTE = 5;
        constexpr std::uint32_t BLEND_CENTER_ATTRIBUTE = 6;
        constexpr std::uint32_t BLEND_ANCHOR0_ATTRIBUTE = 7;
        constexpr std::uint32_t BLEND_ANCHOR1_ATTRIBUTE = 8;
        constexpr std::int32_t METHOD_COMPONENT_COUNT = 1;
        constexpr std::size_t TEXTURE_COORDINATE_OFFSET_MULTIPLIER = 2;
    } // namespace
    platform::RenderResult<GLuint> OpenGL40VertexArrays::create(
        OpenGL40RendererState& runtime, GLuint vertex, GLuint index, GLsizei stride) {
        GLuint vao = NO_GL_OBJECT;
        glGenVertexArrays(RESOURCE_COUNT, &vao);
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vertex);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index);
        glEnableVertexAttribArray(POSITION_ATTRIBUTE);
        glVertexAttribPointer(
            POSITION_ATTRIBUTE, POSITION_COMPONENT_COUNT, GL_FLOAT, GL_FALSE, stride, nullptr);
        if (!(stride < static_cast<GLsizei>(sizeof(types::Vertex)))) {
            glEnableVertexAttribArray(NORMAL_ATTRIBUTE);

            glVertexAttribPointer(NORMAL_ATTRIBUTE,
                NORMAL_COMPONENT_COUNT,
                GL_FLOAT,
                GL_FALSE,
                stride,
                reinterpret_cast<const void*>(sizeof(glm::vec3)));
            glEnableVertexAttribArray(TEXTURE_COORDINATE_ATTRIBUTE);
            glVertexAttribPointer(TEXTURE_COORDINATE_ATTRIBUTE,
                TEXTURE_COORDINATE_COMPONENT_COUNT,
                GL_FLOAT,
                GL_FALSE,
                stride,
                reinterpret_cast<const void*>(
                    sizeof(glm::vec3) * TEXTURE_COORDINATE_OFFSET_MULTIPLIER));
        }
        if (stride == static_cast<GLsizei>(sizeof(SkinnedVertex))) {
            glEnableVertexAttribArray(BLEND_INDEX_ATTRIBUTE);

            glVertexAttribIPointer(BLEND_INDEX_ATTRIBUTE,
                BLEND_INDEX_COMPONENT_COUNT,
                GL_UNSIGNED_INT,
                stride,
                reinterpret_cast<const void*>(offsetof(SkinnedVertex, bones)));
            glEnableVertexAttribArray(BLEND_WEIGHT_ATTRIBUTE);
            glVertexAttribPointer(BLEND_WEIGHT_ATTRIBUTE,
                BLEND_WEIGHT_COMPONENT_COUNT,
                GL_FLOAT,
                GL_FALSE,
                stride,
                reinterpret_cast<const void*>(offsetof(SkinnedVertex, weights)));
            glEnableVertexAttribArray(SKINNING_METHOD_ATTRIBUTE);
            glVertexAttribIPointer(SKINNING_METHOD_ATTRIBUTE,
                METHOD_COMPONENT_COUNT,
                GL_UNSIGNED_INT,
                stride,
                reinterpret_cast<const void*>(offsetof(SkinnedVertex, method)));
            const auto bind_anchor = [stride](GLuint attribute, std::size_t offset) {
                glEnableVertexAttribArray(attribute);
                glVertexAttribPointer(attribute,
                    POSITION_COMPONENT_COUNT,
                    GL_FLOAT,
                    GL_FALSE,
                    stride,
                    reinterpret_cast<const void*>(offset));
            };
            bind_anchor(BLEND_CENTER_ATTRIBUTE, offsetof(SkinnedVertex, center));
            bind_anchor(BLEND_ANCHOR0_ATTRIBUTE, offsetof(SkinnedVertex, anchor0));
            bind_anchor(BLEND_ANCHOR1_ATTRIBUTE, offsetof(SkinnedVertex, anchor1));
        }
        auto layout_result = check_gl_errors("mesh vertex array configuration");
        if (layout_result.is_err()) {
            glDeleteVertexArrays(RESOURCE_COUNT, &vao);
            return std::move(layout_result).take_err();
        }
        runtime.vertex_arrays.emplace_back(vao);
        return vao;
    }
} // namespace enishi::renderer::opengl
