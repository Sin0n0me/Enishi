#include "opengl40_materials.h"
#include <renderer/opengl/opengl40/buffer/opengl40_uniform_buffers.h>
#include <renderer/opengl/opengl40/image/opengl40_images.h>
#include <renderer/opengl/opengl40/runtime/opengl40_handles.h>
namespace enishi::renderer::opengl {
    platform::RenderResult<std::vector<std::vector<types::HandleId>>>
    OpenGL40Materials::make_uniforms(OpenGL40RendererState& runtime,
        ResourceAccessor& resource_accessor,
        const std::vector<types::RenderHandle>& shader_reflections,
        std::vector<types::MeshMaterial>& materials,
        std::vector<types::MeshHandles::UniformBuffers>& material_uniforms) {
        std::vector<std::vector<types::HandleId>> material_uniform_handles;
        for (auto& material : materials) {
            auto& named = material_uniforms.emplace_back();
            if (material.uniforms == nullptr) {
                material_uniform_handles.emplace_back();
                continue;
            }
            auto uniforms = OpenGL40UniformBuffers::make(
                runtime, resource_accessor, shader_reflections, *material.uniforms, named);
            if (uniforms.is_err()) {
                return uniforms.propagation(platform::RenderError::MakeError);
            }
            material_uniform_handles.push_back(std::move(uniforms).unwrap());
        }
        return material_uniform_handles;
    }
    platform::RenderResult<void> OpenGL40Materials::make_bindings(OpenGL40RendererState& runtime,
        RenderHandleMapper& handle_mapper,
        const types::MeshData& data,
        types::RenderHandle handle,
        types::RenderHandle index,
        types::MeshHandles& mesh_handles) {
        std::vector<types::DrawBinding> draw_bindings;
        std::vector<types::RenderHandle> draw_handles;
        std::vector<std::unordered_map<std::uint32_t, std::uint32_t>> texture_bindings;
        draw_bindings.reserve(data.materials.size());
        draw_handles.reserve(data.materials.size());
        texture_bindings.reserve(data.materials.size());
        std::unordered_map<const types::TextureData*, std::uint32_t> texture_objects;
        for (const auto& material : data.materials) {
            draw_bindings.emplace_back(material.draw_binding);
            std::unordered_map<std::uint32_t, std::uint32_t> material_texture_bindings;
            for (const auto& [name, texture] : material.textures) {
                const auto binding = runtime.sampler_bindings.find(name);
                if (binding == runtime.sampler_bindings.end() || texture == nullptr) {
                    continue;
                }
                const auto cached_texture = texture_objects.find(texture.get());
                if (cached_texture != texture_objects.end()) {
                    material_texture_bindings.emplace(binding->second, cached_texture->second);
                    continue;
                }
                const auto created_texture =
                    OpenGL40Images::create_texture(runtime, handle_mapper, *texture);
                if (created_texture.is_err()) {
                    return created_texture.propagation(platform::RenderError::MakeError);
                }
                const auto object = runtime.objects.find(created_texture.unwrap());
                if (object == runtime.objects.end()) {
                    return foundation::Error(
                        platform::RenderError::MakeError, "OpenGL texture object was not created");
                }
                texture_objects.emplace(texture.get(), object->second);
                material_texture_bindings.emplace(binding->second, object->second);
            }
            texture_bindings.emplace_back(std::move(material_texture_bindings));
            const auto draw_handle =
                OpenGL40Handles::make_handle(handle_mapper, types::RenderHandleType::Draw);
            handle_mapper[draw_handle].resource = handle_mapper[index].resource;
            runtime.draw_bindings.emplace(draw_handle, material.draw_binding);
            draw_handles.emplace_back(draw_handle);
        }
        runtime.mesh_draw_bindings.emplace(handle, std::move(draw_bindings));
        runtime.mesh_texture_bindings.emplace(handle, std::move(texture_bindings));
        for (const auto& draw_handle : draw_handles) {
            mesh_handles.mesh_handles.emplace_back(draw_handle);
        }
        return {};
    }
    platform::RenderResult<void> OpenGL40Materials::validate(
        const std::vector<types::MeshMaterial>& materials) {
        constexpr std::uint32_t UNSUPPORTED_FIRST_INSTANCE = 0;
        for (const auto& material : materials) {
            if (const auto indexed =
                    std::get_if<types::DrawIndexedParameter>(&material.draw_binding.parameter);
                indexed != nullptr) {
                if (indexed->first_instance != UNSUPPORTED_FIRST_INSTANCE) {
                    return foundation::Error(platform::RenderError::MakeError,
                        "OpenGL 4.0 does not support non-zero first_instance");
                }
            } else if (const auto plain =
                           std::get_if<types::DrawParameter>(&material.draw_binding.parameter);
                       plain != nullptr) {
                if (plain->first_instance != UNSUPPORTED_FIRST_INSTANCE) {
                    return foundation::Error(platform::RenderError::MakeError,
                        "OpenGL 4.0 does not support non-zero first_instance");
                }
            }
        }
        return {};
    }
} // namespace enishi::renderer::opengl
