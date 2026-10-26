#include "opengl40_views.h"
#include <glad/gl.h>
#include <renderer/opengl/common/opengl_image_view.h>
#include <renderer/opengl/opengl40/runtime/opengl40_handles.h>

namespace enishi::renderer::opengl {
    platform::RenderResult<std::shared_ptr<platform::IRenderTargetView>>
    OpenGL40Views::create_render_target_view(OpenGL40RendererState& runtime,
        ResourceAccessor& resource_accessor,
        RenderHandleMapper& handle_mapper,
        types::RenderHandle image,
        const types::ImageViewDescription& description) {
        if (!runtime.objects.contains(image)) {
            return foundation::Error(platform::RenderError::MakeError, "Image handle is invalid");
        }
        const auto image_description = runtime.images.find(image);
        if (image.type != types::RenderHandleType::Image ||
            image_description == runtime.images.end()) {
            return foundation::Error(platform::RenderError::MakeError,
                "Render target view requires an image description");
        }
        const auto handle =
            OpenGL40Handles::make_handle(handle_mapper, types::RenderHandleType::View);
        runtime.objects.emplace(handle, runtime.objects.at(image));
        runtime.images.emplace(handle, image_description->second);
        if (runtime.back_buffer_images.contains(image)) {
            runtime.back_buffer_images.emplace(handle);
        }
        auto view = std::make_shared<OpenGLRenderTargetView>(handle, description);
        resource_accessor.make_render_target_view(handle.id, std::move(view));
        return resource_accessor.get_render_target_view(handle.id).unwrap();
    }
    platform::RenderResult<std::shared_ptr<platform::IDepthStencilView>>
    OpenGL40Views::create_depth_stencil_view(OpenGL40RendererState& runtime,
        ResourceAccessor& resource_accessor,
        RenderHandleMapper& handle_mapper,
        types::RenderHandle image,
        const types::ImageViewDescription& description) {
        if (!runtime.objects.contains(image)) {
            return foundation::Error(platform::RenderError::MakeError, "Image handle is invalid");
        }
        const auto image_description = runtime.images.find(image);
        if (image_description == runtime.images.end() ||
            image_description->second.format != description.format) {
            return foundation::Error(platform::RenderError::MakeError,
                "Depth stencil view format does not match its image format");
        }
        const auto handle =
            OpenGL40Handles::make_handle(handle_mapper, types::RenderHandleType::View);
        runtime.objects.emplace(handle, runtime.objects.at(image));
        runtime.images.emplace(handle, image_description->second);
        auto view = std::make_shared<OpenGLDepthStencilView>(handle, description);
        resource_accessor.make_depth_stencil_view(handle.id, std::move(view));
        return resource_accessor.get_depth_stencil_view(handle.id).unwrap();
    }
    platform::RenderResult<std::shared_ptr<platform::IShaderResourceView>>
    OpenGL40Views::create_shader_resource_view(OpenGL40RendererState& runtime,
        ResourceAccessor& resource_accessor,
        RenderHandleMapper& handle_mapper,
        types::RenderHandle image,
        const types::ImageViewDescription& description) {
        if (runtime.texture_unit_limit < 0 ||
            !(description.shader_resource_slot <
                static_cast<std::uint32_t>(runtime.texture_unit_limit))) {
            return foundation::Error(platform::RenderError::MakeError,
                "OpenGL shader resource view texture unit exceeds the device limit");
        }
        if (!runtime.objects.contains(image)) {
            return foundation::Error(platform::RenderError::MakeError, "Image handle is invalid");
        }
        const auto handle =
            OpenGL40Handles::make_handle(handle_mapper, types::RenderHandleType::View);
        runtime.objects.emplace(handle, runtime.objects.at(image));
        runtime.view_texture_units.emplace(handle, description.shader_resource_slot);
        auto view = std::make_shared<OpenGLShaderResourceView>(handle, description);
        resource_accessor.make_shader_resource_view(handle.id, std::move(view));
        return resource_accessor.get_shader_resource_view(handle.id).unwrap();
    }
} // namespace enishi::renderer::opengl
