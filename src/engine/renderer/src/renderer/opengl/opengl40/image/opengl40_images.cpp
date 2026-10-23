#include "opengl40_images.h"
#include <algorithm>
#include <cstdint>
#include <glad/gl.h>
#include <renderer/opengl/opengl40/runtime/opengl40_handles.h>
#include <renderer/opengl/opengl40/support/opengl40_errors.h>
#include <renderer/opengl/opengl40/support/opengl40_helpers.h>

namespace enishi::renderer::opengl {
    namespace {
        constexpr std::uint32_t NO_GL_OBJECT = 0;
        constexpr std::uint32_t RESOURCE_COUNT = 1;
        constexpr std::int32_t MINIMUM_IMAGE_DIMENSION = 1;
        constexpr std::uint32_t SINGLE_IMAGE_LAYER = 1;
        constexpr std::uint32_t SINGLE_SAMPLE = 1;
        constexpr std::int32_t MIP_DIMENSION_DIVISOR = 2;
        constexpr std::int32_t TEXTURE_IMAGE_BORDER_WIDTH = 0;
        constexpr std::int32_t UNPACK_ALIGNMENT = 1;
        constexpr std::size_t MIPMAP_COUNT_THRESHOLD = 1;
    } // namespace
    platform::RenderResult<types::RenderHandle> OpenGL40Images::create_image(
        OpenGL40RendererState& runtime,
        RenderHandleMapper& handle_mapper,
        const types::ImageDescription& description) {
        auto pending = check_gl_errors("before image creation");
        if (pending.is_err()) {
            return std::move(pending).take_err();
        }
        if (description.size.x < MINIMUM_IMAGE_DIMENSION ||
            description.size.y < MINIMUM_IMAGE_DIMENSION ||
            description.mip_levels == NO_GL_OBJECT ||
            description.array_layers != SINGLE_IMAGE_LAYER ||
            description.samples != SINGLE_SAMPLE) {
            return foundation::Error(platform::RenderError::MakeError,
                "OpenGL 4.0 supports only single-sample, single-layer 2D images");
        }
        if (description.format == types::ImageFormat::BC3_UNORM ||
            description.format == types::ImageFormat::BC7_UNORM) {
            return foundation::Error(platform::RenderError::MakeError,
                "OpenGL 4.0 image creation does not support compressed formats");
        }
        if (description.contains(types::ImageUsage::BackBuffer)) {
            const auto handle =
                OpenGL40Handles::make_handle(handle_mapper, types::RenderHandleType::Image);
            runtime.images.emplace(handle, description);
            runtime.back_buffer_images.emplace(handle);

            runtime.objects.emplace(handle, NO_GL_OBJECT);
            return handle;
        }
        const auto internal_format = helpers::to_gl_image_internal_format(description.format);
        GLuint texture = NO_GL_OBJECT;
        glGenTextures(RESOURCE_COUNT, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        const auto format = helpers::to_gl_image_format(description.format);
        const auto type = helpers::to_gl_image_type(description.format);
        auto width = description.size.x;
        auto height = description.size.y;
        for (std::uint32_t level = 0; level < description.mip_levels; ++level) {
            glTexImage2D(GL_TEXTURE_2D,
                static_cast<GLint>(level),
                internal_format,
                width,
                height,
                TEXTURE_IMAGE_BORDER_WIDTH,
                format,
                type,
                nullptr);
            width = std::max(MINIMUM_IMAGE_DIMENSION, width / MIP_DIMENSION_DIVISOR);
            height = std::max(MINIMUM_IMAGE_DIMENSION, height / MIP_DIMENSION_DIVISOR);
        }
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(
            GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, static_cast<GLint>(description.mip_levels - 1));
        auto result = check_gl_errors("image allocation");
        if (result.is_err()) {
            glDeleteTextures(RESOURCE_COUNT, &texture);
            return std::move(result).take_err();
        }
        const auto handle =
            OpenGL40Handles::make_handle(handle_mapper, types::RenderHandleType::Image);
        runtime.images.emplace(handle, description);
        runtime.textures.emplace_back(texture);
        runtime.objects.emplace(handle, texture);
        return handle;
    }
    platform::RenderResult<types::RenderHandle> OpenGL40Images::create_texture(
        OpenGL40RendererState& runtime,
        RenderHandleMapper& handle_mapper,
        const types::TextureData& texture) {
        auto pending = check_gl_errors("before texture creation");
        if (pending.is_err()) {
            return std::move(pending).take_err();
        }
        if (texture.is_cubemap || texture.depth > SINGLE_IMAGE_LAYER ||
            texture.array_size != SINGLE_IMAGE_LAYER || texture.mips.empty() ||
            types::TextureData::is_compressed(texture.format)) {
            return foundation::Error(
                platform::RenderError::MakeError, "Unsupported OpenGL 4.0 texture format");
        }
        const auto internal_format = helpers::to_gl_texture_internal_format(texture.format);
        const auto format = helpers::to_gl_texture_format(texture.format);
        const auto type = helpers::to_gl_texture_type(texture.format);
        GLuint object = NO_GL_OBJECT;
        glGenTextures(RESOURCE_COUNT, &object);
        glBindTexture(GL_TEXTURE_2D, object);
        glPixelStorei(GL_UNPACK_ALIGNMENT, UNPACK_ALIGNMENT);
        for (std::uint32_t level = 0; level < texture.mips.size(); ++level) {
            const auto& mip = texture.mips[level];
            glTexImage2D(GL_TEXTURE_2D,
                static_cast<GLint>(level),
                internal_format,
                mip.width,
                mip.height,
                TEXTURE_IMAGE_BORDER_WIDTH,
                format,
                type,
                mip.pixels.data());
        }
        glTexParameteri(GL_TEXTURE_2D,
            GL_TEXTURE_MIN_FILTER,
            texture.mips.size() > MIPMAP_COUNT_THRESHOLD ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D,
            GL_TEXTURE_MAX_LEVEL,
            static_cast<GLint>(texture.mips.size() - MIPMAP_COUNT_THRESHOLD));
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        auto result = check_gl_errors("texture allocation and upload");
        if (result.is_err()) {
            glDeleteTextures(RESOURCE_COUNT, &object);
            return std::move(result).take_err();
        }
        runtime.textures.emplace_back(object);
        const auto handle =
            OpenGL40Handles::make_handle(handle_mapper, types::RenderHandleType::Image);
        runtime.objects.emplace(handle, object);
        return handle;
    }
} // namespace enishi::renderer::opengl
